/*  _        _   _ _ _ _
   / \   ___| |_(_) (_) |_ _   _
  / _ \ / __| __| | | | __| | | |
 / ___ \ (__| |_| | | | |_| |_| |
/_/   \_\___|\__|_|_|_|\__|\__, |
                           |___/
    (C)2017 Actility
License: see LICENCE_SLA0ACT.TXT file include in the project
Description: BSPATCH
*/

#ifndef FOTA_BSPATCH32_H
#define FOTA_BSPATCH32_H

#include <stdint.h>
#include "fota_patch_defs.h"
//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//----------------------------------------------------------------------------
int bspatch(const uint8_t *old, int32_t oldsize,
            uint8_t *new_buf, int32_t newsize,
            uint8_t *patch, int32_t patch_size);

//----------------------------------------------------------------------------
typedef enum { bstControl, bstDiff, bstExtra } bspatch_stage_t;
typedef enum { unsqIdle, unsqByte1, unsqByte2 } unsqueeze_stage_t;
typedef struct {
    const uint8_t *old_start;
    const uint8_t *old_end;
    //
    uint8_t *pold;
    int32_t ctrl[3];
    bspatch_stage_t stage;
    int cnt;
    //
    unsqueeze_stage_t zero_stage;
    unsigned int zero_cnt;
} bspatch_state_t;
fota_patch_result_t bspatch_append(bspatch_state_t *st, uint8_t b);
fota_patch_result_t bspatch_append_squeezed(bspatch_state_t *st, uint8_t b);
//----------------------------------------------------------------------------
#ifdef __cplusplus
} /* extern "C" */
#endif
//----------------------------------------------------------------------------
#endif // FOTA_BSPATCH32_H

