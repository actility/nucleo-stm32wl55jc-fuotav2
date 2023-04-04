/*  _        _   _ _ _ _
   / \   ___| |_(_) (_) |_ _   _
  / _ \ / __| __| | | | __| | | |
 / ___ \ (__| |_| | | | |_| |_| |
/_/   \_\___|\__|_|_|_|\__|\__, |
                           |___/
    (C)2017 Actility
License: see LICENCE_SLA0ACT.TXT file include in the project
Description: FOTA Image Patching Miscellaneous
*/

#ifndef FOTA_MISC_H
#define FOTA_MISC_H
//----------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>

#include "../../src/fota_patch_defs.h"
//----------------------------------------------------------------------------
typedef struct {
    uint8_t *data;
    size_t size;
    int pos;
} bbuf_t;
//----------------------------------------------------------------------------
int packed_name(char *fname_source, char *fname_target, char *sep,
               char *buf, size_t bufsize);
int target_name(char *fname_packed, char *buf, size_t bufsize);

void bbuf_alloc(bbuf_t *bb, size_t size);
void bbuf_free(bbuf_t *bb);
bool bbuf_fread(bbuf_t *bb, char *fname);
bool bbuf_fwrite(bbuf_t *bb, char *fname);

int diff_squeeze(uint8_t *src, size_t src_size, uint8_t *dst, size_t dst_size);
int diff_unsqueeze(uint8_t *src, size_t src_size, uint8_t *dst, size_t dst_size);

char *fota_patch_result_str(fota_patch_result_t code);
//----------------------------------------------------------------------------
#endif // FOTA_MISC_H
