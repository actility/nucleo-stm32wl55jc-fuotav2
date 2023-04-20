/*  _        _   _ _ _ _
   / \   ___| |_(_) (_) |_ _   _
  / _ \ / __| __| | | | __| | | |
 / ___ \ (__| |_| | | | |_| |_| |
/_/   \_\___|\__|_|_|_|\__|\__, |
                           |___/
    (C)2022 Actility
License: see LICENCE_SLA0ACT.TXT file include in the project
Description: FOTA storage handling
*/

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "fota_storage.h"
#include "verify_signature.h"
#include "utilities.h"
#include "sys_app.h"


/* Private variables ---------------------------------------------------------*/
#ifndef SDTEST
__attribute__((aligned (8))) static uint8_t ram_storage[RAM_STORAGE_SZ];
#endif /* SDTEST */
__attribute__((aligned (8))) static uint8_t newimage_buf[NEWIMAGE_BUF_SIZE];

static uint32_t download_start;         /* Start of DOWNLOAD region */
static uint32_t download_end;           /* End of patch in DOWNLOAD region, next byte after DOWNLOAD */
static uint32_t download_ptr;           /* Read pointer in DOWNLOAD region, current byte to read in DOWNLOAD  */
static uint32_t download_download_ptr;  /* Pointer to portion remaining in DOWNLOAD region, current byte to read */
static uint32_t download_scratch_end;   /* Pointer to end of current patch chunk in scratch (SWAP) region, next byte after SCRATCH end */

static uint32_t newimage_start;         /* Start of new image */
static uint32_t newimage_ptr;           /* Pointer to next byte to write image, current NEWIMAGE byte to write */
static uint32_t newimage_buf_ptr;       /* Pointer in the 8 bytes temp buffer which should be flushed to flash when all 8 bytes gathered or
                                           on demand in fota_storage_flush()  */

#ifdef  SDTEST
extern uint8_t sdelta_test_buffer[];
#endif

#define PAGE_INDEX(__ADDRESS__) (uint32_t)((((__ADDRESS__) - FLASH_BASE) % FLASH_BANK_SIZE) / FLASH_PAGE_SIZE) /*!< Get page index from page address */

/* Exported functions --------------------------------------------------------*/

int32_t fota_storage_write(uint8_t *data, uint32_t size, storage_status_t *error) {

  uint32_t write_end = newimage_ptr + size - 1;
  int32_t status = FLASH_IF_OK;
  uint32_t nbcopy;

  if( ((uint32_t)data % FLASH_ALIGN != 0) || (size % FLASH_ALIGN != 0) ) {
    *error = STR_BADALIGN;
    return -1;
  }
  if( download_download_ptr > 0 && write_end >= download_download_ptr ) {
    /* SCRATCH located chunk of patch still not processed and  we haven't space in NEIMAGE */
    *error = STR_NOMEM;
    return -1;
  }
  if( HAL_FLASH_Unlock() != HAL_OK ) {
    *error = STR_HAL_ERR;
    return -1;
  }
  if( write_end >= download_start ) {
    uint32_t nb_pages = PAGE_INDEX(download_end) - PAGE_INDEX(download_ptr) + 1U;
    /* NEWIMAGE overload, copy patch chunk to SCRATCH */
    if( nb_pages > (PAGE_INDEX(FOTA_SWAP_REGION_END) - PAGE_INDEX(FOTA_SWAP_REGION_START) + 1) ) {
      nb_pages = PAGE_INDEX(FOTA_SWAP_REGION_END) - PAGE_INDEX(FOTA_SWAP_REGION_START) + 1;
    }
    if( status == FLASH_IF_OK ) {
      status = FLASH_IF_Erase((void *)FOTA_SWAP_REGION_START, nb_pages * FLASH_PAGE_SIZE);
    }
    if( status == FLASH_IF_OK ) {
      if( (download_end - download_start) > nb_pages * FLASH_PAGE_SIZE ) {
        nbcopy = nb_pages * FLASH_PAGE_SIZE;
        download_scratch_end = FOTA_SWAP_REGION_START + nbcopy;
      } else {
        nbcopy = ((download_end - download_start) / FLASH_ALIGN + 1) * FLASH_ALIGN;
        download_scratch_end = FOTA_SWAP_REGION_START + download_end - download_ptr;
      }
      status = fota_flash_write(FOTA_SWAP_REGION_START, download_ptr, nbcopy);
      download_download_ptr = download_ptr;
      download_ptr = FOTA_SWAP_REGION_START;
    }
    /* Do not forget to cleanup dirty pages of DOWNLOAD region to prepare it for NEWIMAGE */
    if( status == FLASH_IF_OK ) {
      uint32_t endpage = PAGE_INDEX(download_ptr - download_start + nbcopy);
      nb_pages = endpage - PAGE_INDEX(download_start);
      status = FLASH_IF_Erase((void *)download_start, nb_pages * FLASH_PAGE_SIZE);
      download_start = FOTA_DWL_REGION_START + endpage * FLASH_PAGE_SIZE;
    }
  }
  if( status == FLASH_IF_OK ) {
    status = fota_flash_write(newimage_ptr, (uint32_t)data, size);
  }

  HAL_FLASH_Lock();
  if( status != FLASH_IF_OK )  {
    APP_PRINTF("fota_storage_write: newimage_ptr = %x status = %d size = %d\r\n", newimage_ptr, status, size);
    *error = STR_HAL_ERR;
    return -1;
  }
  newimage_ptr += size;
  return size;
}


int32_t fota_storage_read(uint8_t *data, uint32_t size, storage_status_t *error) {

  uint32_t len = 0;
  uint32_t len1 = 0;
  uint8_t *dp = data;

  if( ((uint32_t)data % FLASH_ALIGN != 0) || (size % FLASH_ALIGN != 0) ) {
     *error = STR_BADALIGN;
     return -1;
  }

  if( download_download_ptr > 0 ) {
    /* Here we first process chunk in scratch region */
    len = download_scratch_end - download_ptr;
    UTIL_MEM_cpy_8 (dp, (uint8_t *)download_ptr, len);
    dp += len;
    download_ptr += len;
    if( size >= len ) {
      /* We finished with scratch region */
      size -= len;
      download_ptr = download_download_ptr;
      download_download_ptr = 0;
      download_scratch_end = 0;
    }
  }
  len1 = download_end - download_ptr;

  if( len1 < size ) {
    /* Not enough data remaining in DOWNLOAD region */
    UTIL_MEM_cpy_8 (dp, (uint8_t *)download_ptr, len1);
    download_ptr += len1;
    len += len1;
  } else if( size > 0 ) {
    UTIL_MEM_cpy_8 (dp, (uint8_t *)download_ptr, size);
    len += size;
    download_ptr += size;
  }
  return len;
}

int32_t fota_flash_write(uint32_t addr, uint32_t data, uint32_t size) {
  uint32_t sz;
  int32_t status = FLASH_IF_OK;

  /*
   * FLASH_IF_Write() do not properly support multipage writes
   * with page aligned start and page unaligned end. So we
   * need this lengthy workaround here to cut write into page size chunks
   */
  if (size < FLASH_PAGE_SIZE) {
    sz = size;
  } else {
    sz = FLASH_PAGE_SIZE;
  }
  while (status == FLASH_IF_OK && size > 0) {
    status = FLASH_IF_Write((void *)addr, (uint8_t *)data, sz);
    addr += sz;
    data += sz;
    size -= sz;
    if (size < FLASH_PAGE_SIZE) {
      sz = size;
    }
  }
  return status;
}

int32_t fota_storage_init(uint32_t size, storage_status_t *error) {

  /* We must have patch ending at 64 bit boundary */
  uint32_t patch_size = ((size - SMARTDELTA_MAC_LEN) / FLASH_ALIGN + 1) * FLASH_ALIGN;
  uint32_t first_page = PAGE_INDEX(FOTA_DWL_REGION_START);
  uint32_t nb_pages = PAGE_INDEX(FOTA_DWL_REGION_START + size - 1) - first_page + 1U;
  int32_t status = FLASH_IF_OK;

  download_start = FOTA_DWL_REGION_START;
  download_ptr = FLASH_BASE + PAGE_INDEX(FOTA_DWL_REGION_END - patch_size + 1) * FLASH_PAGE_SIZE; /* Patch will be aligned on the page boundary */
  download_end = download_ptr + size - SMARTDELTA_MAC_LEN; /* TODO: check what will be if size 64 bit unaligned. without signature */

  if( PAGE_INDEX(download_ptr) == PAGE_INDEX(download_start) + 1) { /* +1 because we will erase below one more page after patch end */
    *error = STR_NOMEM;
    return -1;
  }

  if( HAL_FLASH_Unlock() == HAL_OK) {
      status = fota_flash_write(download_ptr, download_start, patch_size);
  } else {
    status = FLASH_IF_LOCK_ERROR;
  }
  if( status == FLASH_IF_OK ) {
    /*
     * nb_pages+1 because we will erase one more page after patch end
     * to account for situation when patch size is below page boundary but
     * received fragments file size is above page boundary
     */
    status = FLASH_IF_Erase((void *)download_start, (nb_pages + 1) * FLASH_PAGE_SIZE);
  }
  /* We have to restore image header at the beginning of DOWNLOAD region */
  if( status == FLASH_IF_OK ) {
    status = fota_flash_write(download_start, download_ptr, SFU_IMG_IMAGE_OFFSET);
  }
  HAL_FLASH_Lock();
  if( status != FLASH_IF_OK ) {
    *error = STR_HAL_ERR;
    return -1;
  }

  download_start = download_ptr; /* will be at the patch beginning (image header) */
  download_ptr += SFU_IMG_IMAGE_OFFSET;
  download_download_ptr = 0;
  download_scratch_end = 0;

  newimage_start = FOTA_DWL_REGION_START + SFU_IMG_IMAGE_OFFSET;
  newimage_ptr = newimage_start;
  newimage_buf_ptr = 0;

  *error = STR_OK;
  return 0;
}


uint32_t fota_storage_get_rambuf(uint8_t **ram_buf) {

#ifdef SDTEST
  *ram_buf = &sdelta_test_buffer[0];
#else
  *ram_buf = &ram_storage[0];
#endif /* SDTEST */
  return RAM_STORAGE_SZ;
}

int32_t fota_storage_write_byte (uint8_t b, storage_status_t *error) {

  if (newimage_buf_ptr == 0) {
    newimage_buf_ptr = newimage_ptr;
  }
  newimage_buf[newimage_buf_ptr & (NEWIMAGE_BUF_SIZE - 1)] = b;

  if((newimage_buf_ptr & (NEWIMAGE_BUF_SIZE - 1)) == NEWIMAGE_BUF_SIZE - 1) {
    if(fota_storage_write(newimage_buf, NEWIMAGE_BUF_SIZE, error) < 0) {
        return -1;
    }
    newimage_buf_ptr = 0;
  } else {
    newimage_buf_ptr++;
  }
  return 0;
}

int32_t fota_storage_flush( storage_status_t *error ) {

  uint32_t pos = newimage_buf_ptr & 0x7;
  int32_t status = FLASH_IF_OK;

  if( pos != 0 ) {
    memset(&newimage_buf[pos], 0xFF, 8 - pos);
    if(fota_storage_write(newimage_buf, NEWIMAGE_BUF_SIZE, error) < 0) {
      return -1;
    }
  }
  /**
   * Erase unused N pages in DOWNLOAD/NEWIMAGE region.
   * Tail of the page with current "newimage_ptr" was already erased
   **/
  if( PAGE_INDEX(newimage_ptr) < PAGE_INDEX(FOTA_DWL_REGION_END) ) {
    if( HAL_FLASH_Unlock() == HAL_OK) {
      uint32_t nb_pages = PAGE_INDEX(FOTA_DWL_REGION_END) - PAGE_INDEX(newimage_ptr);
      status = FLASH_IF_Erase((void *)((PAGE_INDEX(newimage_ptr) + 1) * FLASH_PAGE_SIZE + FLASH_BASE), nb_pages * FLASH_PAGE_SIZE);
    }
  }
  if( status != FLASH_IF_OK ) {
    *error = STR_HAL_ERR;
    return -1;
  }
  return 8 - pos;
}

uint32_t fota_storage_get_active_start(void) {
  return FOTA_ACT_REGION_START;
}

uint32_t fota_storage_get_active_len(void) {
  return FOTA_ACT_REGION_SIZE;
}
