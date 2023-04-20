/*  _        _   _ _ _ _
   / \   ___| |_(_) (_) |_ _   _
  / _ \ / __| __| | | | __| | | |
 / ___ \ (__| |_| | | | |_| |_| |
/_/   \_\___|\__|_|_|_|\__|\__, |
                           |___/
    (C)2017 Actility
License: see LICENCE_SLA0ACT.TXT file include in the project
Description: LZG Wrappers
*/

#include "fota_decompress_lzg.h"
#include "fota_patch_stream.h"
//----------------------------------------------------------------------------
/* LUT for decoding the copy length parameter */
static const unsigned char _LZG_LENGTH_DECODE_LUT[32] = {
    2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,
    18,19,20,21,22,23,24,25,26,27,28,29,35,48,72,128
};
//----------------------------------------------------------------------------
fota_patch_result_t decode_write(struct fota_patch_stream_state_s *pstream, uint8_t b) {
    lzg_decode_state_t *lst = &pstream->lzgst;
    lst->hist_buf[lst->hist_buf_pos] = b;
    if(++lst->hist_buf_pos >= lst->window_size) {
        lst->hist_buf_pos = 0;
        lst->hist_buf_full = true;
    }

    return fota_write_decompressed(pstream,b);
}
//----------------------------------------------------------------------------
fota_patch_result_t fotaDecodeLZG_append(struct fota_patch_stream_state_s *pstream, uint8_t b) {
    lzg_uint32_t  length = 0, offset = 0;
    lzg_decode_state_t *lst = &pstream->lzgst;

    switch(lst->stage) {
    case ldMarkers:
        if(lst->cnt < sizeof(lst->marker) / sizeof(*lst->marker)) {
            lst->marker[lst->cnt++] = b;
            return fotaOk;
        }
        lst->stage = ldLiteral;
        // fall through
    case ldLiteral:
        if(b == lst->marker[0]) {
            lst->stage = ldMark1;
            lst->cnt = 0;
            return fotaOk;
        }
        else if(b == lst->marker[1]) {
            lst->stage = ldMark2;
            lst->cnt = 0;
            return fotaOk;
        }
        else if(b == lst->marker[2]) {
            lst->stage = ldMark3;
            lst->cnt = 0;
            return fotaOk;
        }
        else if(b == lst->marker[3]) {
            lst->stage = ldMark4;
            lst->cnt = 0;
            return fotaOk;
        }
        return decode_write(pstream,b);

    case ldMark1:
        if(lst->cnt == 0 && b == 0) { /* Single occurance of a marker symbol... */
            lst->stage = ldLiteral;
            return decode_write(pstream,lst->marker[0]);
        }
        if(lst->cnt < 2) {
            lst->bytes[lst->cnt++] = b;
            return fotaOk;
        }
        /* Distant copy */
        length = _LZG_LENGTH_DECODE_LUT[lst->bytes[0] & 0x1f];
        offset = (((unsigned int)(lst->bytes[0] & 0xe0)) << 11) |
                  (((unsigned int)lst->bytes[1]) << 8) |
                  b;
        offset += 2056;
        break;

    case ldMark2:
        if(lst->cnt == 0 && b == 0) { /* Single occurance of a marker symbol... */
            lst->stage = ldLiteral;
            return decode_write(pstream,lst->marker[1]);
        }
        if(lst->cnt < 1) {
            lst->bytes[lst->cnt++] = b;
            return fotaOk;
        }
        /* Medium copy */
        length = _LZG_LENGTH_DECODE_LUT[lst->bytes[0] & 0x1f];
        offset = (((unsigned int)(lst->bytes[0] & 0xe0)) << 3) | b;
        offset += 8;
        break;

    case ldMark3:
        if(lst->cnt == 0 && b == 0) { /* Single occurance of a marker symbol... */
            lst->stage = ldLiteral;
            return decode_write(pstream,lst->marker[2]);
        }
        /* Short copy */
        length = (b >> 6) + 3;
        offset = (b & 0x3f) + 8;
        break;

    case ldMark4:
        if(lst->cnt == 0 && b == 0) { /* Single occurance of a marker symbol... */
            lst->stage = ldLiteral;
            return decode_write(pstream,lst->marker[3]);
        }
        /* Near copy (including RLE) */
        length = _LZG_LENGTH_DECODE_LUT[b & 0x1f];
        offset = (b >> 5) + 1;
        break;
    }

    /* Copy corresponding data from history window */
    if(offset > lst->window_size) {
        return fotaLzgMiss;
    }
    if(length > lst->window_size) {
        return fotaLzgMiss;
    }
    lzg_int32_t copy_pos = lst->hist_buf_pos - offset;
    if(copy_pos < 0) { copy_pos += lst->window_size; }

    while(length-- > 0) {
        fota_patch_result_t res = decode_write(pstream,lst->hist_buf[copy_pos]);
        if(res != fotaOk) {
            return res;
        }
        if(++copy_pos >= (int)lst->window_size) {
            copy_pos = 0;
        }
    }

    lst->stage = ldLiteral;
    return fotaOk;
}
//----------------------------------------------------------------------------
