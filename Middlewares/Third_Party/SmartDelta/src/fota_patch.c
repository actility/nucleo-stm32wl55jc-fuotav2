/*  _        _   _ _ _ _
   / \   ___| |_(_) (_) |_ _   _
  / _ \ / __| __| | | | __| | | |
 / ___ \ (__| |_| | | | |_| |_| |
/_/   \_\___|\__|_|_|_|\__|\__, |
                           |___/
    (C)2017 Actility
License: see LICENCE_SLA0ACT.TXT file include in the project
Description: FOTA Firmware Patching Support
*/

#include "fota_patch.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "fota_storage.h"
#include "fota_patch_defs.h"
#include "fota_patch_stream.h"

#include "verify_signature.h"
//----------------------------------------------------------------------------

/**
  * @brief Smart Delta patch verify header function.
  *        Verify that firmware has Smart Delta header.
  * @param fwimagebody : Pointer to firmware body with Smart Delta header.
  * @retval SMARTDELTA_OK if successful, SMARTDELTA_ERROR otherwise.
  */
int32_t fota_patch_verify_header (const uint8_t * fwimagebody) {

	if( memcmp( fwimagebody, FOTA_SIGNATURE_IMG1, FOTA_SIGNATURE_SIZE ) == 0
	 || memcmp( fwimagebody, FOTA_SIGNATURE_PATCH1, FOTA_SIGNATURE_SIZE ) == 0) {
		return SMARTDELTA_OK;
	}
	return SMARTDELTA_ERROR;
}
//----------------------------------------------------------------------------
fota_patch_result_t fota_patch(uint32_t len) {
    
    __attribute__((aligned (8))) static uint8_t temp_buf[64];

    storage_status_t error;
    uint8_t * ram_buf;
    uint32_t rbsz;
    fota_patch_stream_state_t stream;
    size_t patch_header_size;
    fota_patch_result_t pres;
    int32_t blen;
    uint32_t bpos;

    rbsz = fota_storage_get_rambuf(&ram_buf);

    if( fota_storage_init(len, &error) ) {
      return fotaTargetWriteFailed;
    }

    if( (blen = fota_storage_read(temp_buf, sizeof(temp_buf), &error)) != sizeof(temp_buf)) {
      return fotaHeaderSmall;
    }
    pres = fota_patch_stream_init(&stream,
                           temp_buf, sizeof(temp_buf),
                           ram_buf, rbsz,
                           (uint8_t *)FOTA_ACT_REGION_START + SFU_IMG_IMAGE_OFFSET, FOTA_ACT_REGION_SIZE - SFU_IMG_IMAGE_OFFSET,
                           &patch_header_size);
    if(pres != fotaOk) {
       return pres;
    }

    bpos = patch_header_size;     // but bpos should be at patch_header_size !!!

    do {
        for(; bpos < blen; ++bpos) {
            if( (pres = fota_patch_stream_append(&stream, temp_buf[bpos])) != fotaOk ) {
              return pres;
            }
        }
        // processing finished
        if( (blen = fota_storage_read(temp_buf, sizeof(temp_buf), &error)) < 0 ) {
          return fotaError;
        }
        bpos = 0;
    } while(blen != 0);

    if( fota_storage_flush(&error) < 0 ) {
        return fotaTargetWriteFailed;
    } else {
        return fotaOk;
    }
}
//----------------------------------------------------------------------------
