/*  _        _   _ _ _ _
   / \   ___| |_(_) (_) |_ _   _
  / _ \ / __| __| | | | __| | | |
 / ___ \ (__| |_| | | | |_| |_| |
/_/   \_\___|\__|_|_|_|\__|\__, |
                           |___/
    (C)2017 Actility
License: see LICENCE_SLA0ACT.TXT file include in the project
Description: LZH Wrappers
*/

#ifndef FOTA_DECOMPRESS_LZG_H
#define FOTA_DECOMPRESS_LZG_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "fota_patch_defs.h"
//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//----------------------------------------------------------------------------
#define LZG_VERSION "1.0.10"   /**< @brief LZG library version string */

/* Basic types */
typedef int          lzg_bool_t;   /**< @brief Boolean (@ref LZG_TRUE/@ref LZG_FALSE) */
typedef int          lzg_int32_t;  /**< @brief Signed 32-bit integer */
typedef unsigned int lzg_uint32_t; /**< @brief Unsigned 32-bit integer */

//----------------------------------------------------------------------------
typedef enum { ldMarkers, ldLiteral, ldMark1, ldMark2, ldMark3, ldMark4,  } lzg_decode_stage_t;
struct fota_patch_stream_state_s;
typedef struct {
    size_t window_size;
    uint8_t marker[4];
    lzg_decode_stage_t stage;
    unsigned int cnt;
    uint8_t bytes[2];
    bool hist_buf_full;
    unsigned int hist_buf_pos;
    uint8_t *hist_buf;
    size_t hist_buf_size;

} lzg_decode_state_t;

fota_patch_result_t fotaDecodeLZG_append(struct fota_patch_stream_state_s *pstream, uint8_t b);
//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------
#endif // FOTA_DECOMPRESS_LZG_H
