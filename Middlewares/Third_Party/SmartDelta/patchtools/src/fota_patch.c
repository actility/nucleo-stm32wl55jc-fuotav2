/*  _        _   _ _ _ _
   / \   ___| |_(_) (_) |_ _   _
  / _ \ / __| __| | | | __| | | |
 / ___ \ (__| |_| | | | |_| |_| |
/_/   \_\___|\__|_|_|_|\__|\__, |
                           |___/
    (C)2017 Actility
License: see LICENCE_SLA0ACT.TXT file include in the project
Description: FOTA Image Patching Routines
*/

#include "fota_patch.h"
#include "fota_patch_stream.h"
//----------------------------------------------------------------------------
fota_patch_result_t fota_patch(uint8_t *ppatch, size_t patch_size,
                               uint8_t *pram, size_t ram_size,
                               uint8_t *porig, size_t orig_room) {

    fota_patch_stream_state_t stream;
    size_t patch_header_size;

    fota_patch_result_t res = fota_patch_stream_init(&stream,
                                                     ppatch,patch_size,
                                                     pram,ram_size,
                                                     porig,orig_room,
                                                     &patch_header_size);
    if(res != fotaOk) {
        return res;
    }

//printf("  uncompress=%d unsqueeze=%d undiff=%d\n",stream.uncompress,stream.unsqueeze,stream.undiff);

    uint8_t *ppatch_end = ppatch + patch_size;
    for(uint8_t *p = ppatch + patch_header_size; p < ppatch_end; ++p) {
        res = fota_patch_stream_append(&stream,*p);
        if(res != fotaOk) {
            return res;
        }
    }
    return fotaOk;
}
//----------------------------------------------------------------------------
