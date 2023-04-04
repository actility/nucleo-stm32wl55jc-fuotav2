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

#ifndef FOTA_PATCH_H
#define FOTA_PATCH_H
//----------------------------------------------------------------------------
#include <stddef.h>
#include <stdint.h>
//#include <stdbool.h>

#include "fota_patch_defs.h"
//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//----------------------------------------------------------------------------
fota_patch_result_t fota_patch(uint8_t *ppatch, size_t patch_size,
                               uint8_t *pram, size_t ram_size,
                               uint8_t *porig, size_t orig_room);
//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------
#endif // FOTA_PATCH_H
