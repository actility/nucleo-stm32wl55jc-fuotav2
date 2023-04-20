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

/*-
 * Copyright 2003-2005 Colin Percival
 * Copyright 2012 Matthew Endsley
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <string.h>

#include "fota_bspatch32.h"
#include "fota_storage.h"
//----------------------------------------------------------------------------
int32_t offtin32(uint8_t *buf)
{
  int32_t y;
  y = 0;
  y |= (int32_t)(*buf++) << 0;
  y |= (int32_t)(*buf++) << 8;
  y |= (int32_t)(*buf++) << 16;
  y |= (int32_t)(*buf++) << 24;

	return y;
}
//----------------------------------------------------------------------------
__attribute__((weak)) void debug_write_unsqueezed(uint8_t b) {
    (void) b;
}
//----------------------------------------------------------------------------
fota_patch_result_t bspatch_append(bspatch_state_t *st, uint8_t b) {
    debug_write_unsqueezed(b);
    storage_status_t error;

    while(1) {
        switch(st->stage) {
        case bstControl:
            if(st->cnt == 0) { st->ctrl[2] = st->ctrl[1] = st->ctrl[0] = 0; }
            st->ctrl[st->cnt >> 2] |= b << (8 * (st->cnt & 0x03));
            if(++st->cnt >= 12) {
                st->stage = bstDiff;
                st->cnt = st->ctrl[0];
            }
            return fotaOk;
        case bstDiff:
            if(--st->cnt >= 0) {
                if(st->pold < st->old_start || st->pold >= st->old_end ) {
                    return fotaBspatchOrigMiss;
                }
                if(fota_storage_write_byte((*st->pold++) + b, &error) != 0) {
                  return fotaTargetWriteFailed;
                } else {
                  return fotaOk;
                }
            }
            st->stage = bstExtra;
            st->cnt = st->ctrl[1];
            // fall through
        case bstExtra:
            if(--st->cnt >= 0) {
              if(fota_storage_write_byte(b, &error) != 0) {
                  return fotaTargetWriteFailed;
              } else {
                  return fotaOk;
              }
            }
            st->stage = bstControl;
            st->cnt = 0;
            st->pold += st->ctrl[2];
            //goto bstControl;
        }
    }
}
//----------------------------------------------------------------------------
fota_patch_result_t bspatch_append_squeezed(bspatch_state_t *st, uint8_t b) {
    switch(st->zero_stage) {

    case unsqIdle:
        if(b == 0) {
            st->zero_stage = unsqByte1;
            st->zero_cnt = 0;
        }
        return bspatch_append(st,b);

    case unsqByte1:
        if(b & 0x80) {
            st->zero_cnt = (b & ~0x80) << 8;
            st->zero_stage = unsqByte2;
            return fotaOk;
        }
        st->zero_cnt = b;
        break;

    case unsqByte2:
        st->zero_cnt |= b;
        break;
    }

    while(st->zero_cnt--) {
        fota_patch_result_t res = bspatch_append(st,0);
        if(res != fotaOk) {
            return res;
        }
    }
    st->zero_stage = unsqIdle;
    return fotaOk;
}
//----------------------------------------------------------------------------
int bspatch(const uint8_t *old, int32_t oldsize,
            uint8_t *new_buf, int32_t newsize,
            uint8_t *patch, int32_t patch_size) {

  int64_t oldpos,newpos,patchpos;
  int32_t ctrl[3];
  int32_t i;

  oldpos=0;newpos=0;patchpos=0;
  while(newpos<newsize) {
    /* Read control data */
    if(patchpos + 3 * 4 > patch_size) {
        return -1;
    }
    ctrl[0] = offtin32(&patch[patchpos]);
    patchpos += 4;
    ctrl[1] = offtin32(&patch[patchpos]);
    patchpos += 4;
    ctrl[2] = offtin32(&patch[patchpos]);
    patchpos += 4;

    /* Sanity-check */
    if(newpos+ctrl[0]>newsize)
      return -1;

    /* Read diff string */
    if(patchpos + ctrl[0] > patch_size) {
        return -1;
    }
    memcpy(new_buf + newpos,patch + patchpos,ctrl[0]);
    patchpos += ctrl[0];

    /* Add old data to diff string */
    for(i=0;i<ctrl[0];i++)
      if((oldpos+i>=0) && (oldpos+i<oldsize))
        new_buf[newpos+i]+=old[oldpos+i];

    /* Adjust pointers */
    newpos+=ctrl[0];
    oldpos+=ctrl[0];

    /* Sanity-check */
    if(newpos+ctrl[1]>newsize)
      return -1;

    /* Read extra string */
    if(patchpos + ctrl[1] > patch_size) {
        return -1;
    }
    memcpy(new_buf + newpos,patch + patchpos,ctrl[1]);
    patchpos += ctrl[1];

    /* Adjust pointers */
    newpos+=ctrl[1];
    oldpos+=ctrl[2];
  };

	return 0;
}
//----------------------------------------------------------------------------
