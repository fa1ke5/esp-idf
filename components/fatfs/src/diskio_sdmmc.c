// Copyright 2015-2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "diskio.h"
#include "ffconf.h"
#include "ff.h"
#include "sdmmc_cmd.h"
#include "esp_log.h"

#include "string.h"
#define BUF_SIZE 32
//FF_MIN_SS = 512 /*sec size*/
BYTE bBuf[FF_MIN_SS*BUF_SIZE];


DWORD StartSect = 0;
DWORD EndSect = 0;
bool NeedRefresh = true;


static sdmmc_card_t* s_cards[FF_VOLUMES] = { NULL };

static const char* TAG = "diskio_sdmmc";

DSTATUS ff_sdmmc_initialize (BYTE pdrv)
{
    return 0;
}

DSTATUS ff_sdmmc_status (BYTE pdrv)
{
    return 0;
}

DRESULT ReadBuf(BYTE pdrv, BYTE* bBuf, DWORD sector, BYTE* buff){

    sdmmc_card_t* card = s_cards[pdrv];
    assert(card);
    esp_err_t err = sdmmc_read_sectors(card, bBuf, sector, BUF_SIZE); //read BUF_SIZE sectors to bBuf buffer
    if (err != ESP_OK) {
    ESP_LOGE(TAG, "sdmmc_read_blocks failed (%d)", err);
    return RES_ERROR;
                        }
    memcpy(buff, &bBuf[0], FF_MIN_SS); //send 1st sector out
    NeedRefresh = false;
    StartSect = sector; //mem start sector number
    EndSect = sector + BUF_SIZE - 1;
    return RES_OK; //stop and return

}

DRESULT ff_sdmmc_read (BYTE pdrv, BYTE* buff, DWORD sector, UINT count)
{
if(count > 1){
ESP_LOGE(TAG, "WARNING!!! Buffer unconsistent, must be 1, but now > 1 check fatfs lib version, count =  (%d)", count);
}
if(NeedRefresh){

esp_err_t err = ReadBuf(pdrv, bBuf, sector, buff);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "sdmmc_buffer_renew_failed (%d)", err);
        return RES_ERROR;}
    return RES_OK; //stop and return
  }else if(sector > StartSect && sector <= EndSect )
          { //if sector exist in buffer

            //ESP_LOGE(TAG, "Sector in range, sector = (%ld)", EndSect);
          int i = (sector - StartSect)*FF_MIN_SS; //find sector address
          memcpy(buff, &bBuf[i], FF_MIN_SS); //send sector out
          return RES_OK; //stop and return

          }else{
       
    esp_err_t err = ReadBuf(pdrv, bBuf, sector, buff);
    if (err != ESP_OK) {
    ESP_LOGE(TAG, "sdmmc_read_to_buffer failed (%d)", err);
    return RES_ERROR;}
    return RES_OK; //stop and return
            
            }

return RES_OK; //stop and return
  
}

DRESULT ff_sdmmc_write (BYTE pdrv, const BYTE* buff, DWORD sector, UINT count)
{
    sdmmc_card_t* card = s_cards[pdrv];
    assert(card);
    esp_err_t err = sdmmc_write_sectors(card, buff, sector, count);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "sdmmc_write_blocks failed (%d)", err);
        return RES_ERROR;
    }
    return RES_OK;
}

DRESULT ff_sdmmc_ioctl (BYTE pdrv, BYTE cmd, void* buff)
{
    sdmmc_card_t* card = s_cards[pdrv];
    assert(card);
    switch(cmd) {
        case CTRL_SYNC:
            return RES_OK;
        case GET_SECTOR_COUNT:
            *((DWORD*) buff) = card->csd.capacity;
            return RES_OK;
        case GET_SECTOR_SIZE:
            *((WORD*) buff) = card->csd.sector_size;
            return RES_OK;
        case GET_BLOCK_SIZE:
            return RES_ERROR;
    }
    return RES_ERROR;
}

void ff_diskio_register_sdmmc(BYTE pdrv, sdmmc_card_t* card)
{
    static const ff_diskio_impl_t sdmmc_impl = {
        .init = &ff_sdmmc_initialize,
        .status = &ff_sdmmc_status,
        .read = &ff_sdmmc_read,
        .write = &ff_sdmmc_write,
        .ioctl = &ff_sdmmc_ioctl
    };
    s_cards[pdrv] = card;
    ff_diskio_register(pdrv, &sdmmc_impl);
}

