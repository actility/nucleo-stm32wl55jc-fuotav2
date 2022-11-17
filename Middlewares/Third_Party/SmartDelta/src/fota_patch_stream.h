/*  _        _   _ _ _ _
   / \   ___| |_(_) (_) |_ _   _
  / _ \ / __| __| | | | __| | | |
 / ___ \ (__| |_| | | | |_| |_| |
/_/   \_\___|\__|_|_|_|\__|\__, |
                           |___/
    (C)2017 Actility
License: see LICENCE_SLA0ACT.TXT file include in the project
Description: Firmware Patching Stream support
*/

#ifndef FOTA_PATCH_STREAM_H
#define FOTA_PATCH_STREAM_H
//----------------------------------------------------------------------------
#include <stddef.h>
#include <stdint.h>

#include "fota_patch_defs.h"
#include "fota_decompress_lzg.h"
#include "fota_bspatch32.h"
//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//----------------------------------------------------------------------------
typedef struct fota_patch_stream_state_s {
    bool uncompress:1;
    bool unsqueeze:1;
    bool undiff:1;
    lzg_decode_state_t lzgst;
    bspatch_state_t bst;
} fota_patch_stream_state_t;

fota_patch_result_t fota_patch_stream_init(fota_patch_stream_state_t *pstream,
                                           uint8_t *ppatch, size_t patch_filled,
                                           uint8_t *pram, size_t ram_size,
                                           uint8_t *porig, size_t orig_room,
                                           size_t *ppatch_header_size);

fota_patch_result_t fota_patch_stream_append(fota_patch_stream_state_t *pstream, uint8_t byte);
//----------------------------------------------------------------------------
fota_patch_result_t fota_write_decompressed(fota_patch_stream_state_t *pstream, uint8_t b);
//----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------
#endif // FOTA_PATCH_STREAM_H
