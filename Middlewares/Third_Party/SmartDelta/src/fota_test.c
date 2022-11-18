/*  _        _   _ _ _ _
   / \   ___| |_(_) (_) |_ _   _
  / _ \ / __| __| | | | __| | | |
 / ___ \ (__| |_| | | | |_| |_| |
/_/   \_\___|\__|_|_|_|\__|\__, |
                           |___/
    (C)2022 Actility
License: see LICENCE_SLA0ACT.TXT file include in the project
Description: Smart Delta patching test
*/

#include "fota_patch_defs.h"
#include "fota_storage.h"
#include "fota_patch.h"
#include "sys_app.h"
#include "frag_decoder_if.h"
#include "verify_signature.h"
#include "se_def_metadata.h"

void test_runner(void);

static void crc32(const void *data, size_t n_bytes, uint32_t* crc);

#define PATCH_SIZE      5000
#define FRAG_SIZE       216
uint32_t Patch_Size = PATCH_SIZE;
__attribute__((aligned (8))) uint8_t sdelta_test_buffer[((PATCH_SIZE/8) + 1) * 8];

void test_runner(void) {

  uint32_t size = Patch_Size;
  uint32_t fwsize;
  uint32_t fgsize;
  int32_t status = FLASH_IF_OK;
  uint8_t *datafile = (uint8_t *)FOTA_DWL_REGION_START;
  fota_patch_result_t patch_res = fotaError;

  fgsize = ((size - 1)/FRAG_SIZE + 1) * FRAG_SIZE;// round to fragment boundary up
  fwsize = ((fgsize - 1)/8 + 1) * 8; // round to 8 bytes boundary
  memset(sdelta_test_buffer, 0, fwsize);
  if ((status = FRAG_DECODER_IF_Erase()) == FLASH_IF_OK) {
    if( HAL_FLASH_Unlock() == HAL_OK) {
      status = fota_flash_write(FOTA_DWL_REGION_START, (uint32_t)sdelta_test_buffer, fwsize);
    } else {
      status = FLASH_IF_LOCK_ERROR;
    }
  }
  if (status == FLASH_IF_OK) {
    if( *(uint32_t *)datafile != FIRMWARE_MAGIC)
    {
      APP_PRINTF("Binary file received, no firmware magic found\r\n");
    } else {
      datafile += SFU_IMG_IMAGE_OFFSET;
      if( size <= SFU_IMG_IMAGE_OFFSET )
      {
        APP_PRINTF("File size: %u less then: %u error\r\n", size, SFU_IMG_IMAGE_OFFSET);
      } else {
        if( fota_patch_verify_header(datafile) == SMARTDELTA_OK )
        {
          fwsize = size - SFU_IMG_IMAGE_OFFSET;
          if( 1 /* fota_patch_verify_signature (datafile, fwsize) == SMARTDELTA_OK */ ) {

            APP_PRINTF("Patch size: %u\r\n", size);
            patch_res = fota_patch(size);
            if (patch_res == fotaOk) {
              datafile -= SFU_IMG_IMAGE_OFFSET;
              fwsize = ((SE_FwRawHeaderTypeDef *)datafile)->FwSize;
              uint32_t crc = 0;
              crc32(datafile + SFU_IMG_IMAGE_OFFSET, fwsize, &crc);
              APP_PRINTF("\r\n...... Smart Delta to Flash Succeeded crc: %x ......\r\n", crc);
            } else if (patch_res == fotaHeaderSignatureUnsupported) {
              APP_PRINTF("...... Patch unrecognized ......\r\n");
            } else {
              APP_PRINTF("Patch error:%d\r\n", patch_res);
            }
          } else {
            APP_PRINTF("Invalid Smart Delta signature\r\n");
          }
        } else {
          APP_PRINTF("Full image received\r\n");
        }
      }
    }
  } else {
    APP_PRINTF("Flash preparation failed: %d\r\n", status);
  }
  if (patch_res != fotaOk) {
    while(1);
  }

}

static uint32_t crc32_for_byte(uint32_t r) {
  for(int j = 0; j < 8; ++j)
    r = (r & 1? 0: (uint32_t)0xEDB88320L) ^ r >> 1;
  return r ^ (uint32_t)0xFF000000L;
}

static void crc32(const void *data, size_t n_bytes, uint32_t* crc) {
  static uint32_t table[0x100];
  if(!*table)
    for(size_t i = 0; i < 0x100; ++i)
      table[i] = crc32_for_byte(i);
  for(size_t i = 0; i < n_bytes; ++i)
    *crc = table[(uint8_t)*crc ^ ((uint8_t*)data)[i]] ^ *crc >> 8;
}
