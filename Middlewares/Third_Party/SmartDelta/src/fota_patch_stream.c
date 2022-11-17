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

#include "fota_patch.h"
#include "fota_patch_stream.h"
#include "fota_patch_hash.h"
#include "fota_storage.h"

//----------------------------------------------------------------------------
__attribute__((weak)) void debug_write_decompressed(uint8_t b) {
    (void) b;
}
//----------------------------------------------------------------------------
static int sigcmp(uint8_t *psig1, uint8_t *psig2) {
    for(int i = FOTA_SIGNATURE_SIZE; i > 0; --i ) {
        if(*psig1++ != *psig2++) {
            return 0;
        }
    }
    return 1;
}

//----------------------------------------------------------------------------
fota_patch_result_t fota_write_decompressed(fota_patch_stream_state_t *pstream, uint8_t b) {
    debug_write_decompressed(b);
    storage_status_t error;

    if(pstream->undiff) {
        if(pstream->unsqueeze) {
            return bspatch_append_squeezed(&pstream->bst,b);
        } else {
            return bspatch_append(&pstream->bst,b);
        }
    }
    if(fota_storage_write_byte(b, &error) != 0) {
        return fotaTargetWriteFailed;
    } else {
        return fotaOk;
    }
}
//----------------------------------------------------------------------------
fota_patch_result_t fota_patch_stream_init(fota_patch_stream_state_t *pstream,
                                           uint8_t *ppatch, size_t patch_filled,
                                           uint8_t *pram, size_t ram_size,
                                           uint8_t *porig, size_t orig_room,
                                           size_t *ppatch_header_size) {

    if(patch_filled <= FOTA_SIGNATURE_SIZE) {
        return fotaHeaderSignatureSmall;
    }

    if(sigcmp(ppatch,(uint8_t *)FOTA_SIGNATURE_PATCH1)) {
        if(patch_filled < sizeof(fota_header_fotap1_t)) {
            return fotaHeaderSmall;
        }
        pstream->undiff = true;
        fota_header_fotap1_t *php1 = (fota_header_fotap1_t *)ppatch;
        pstream->lzgst.window_size = 1 << php1->PackWindowOrderP1;
        pstream->unsqueeze = !php1->ZeroNotSqueezed;
        if(php1->flags2 != 0) {
            return fotaHeaderParsBad;
        }

        if(orig_room < php1->OriginalLength) {
            return fotaPatchOrigSmall;
        }
        uint32_t orig_hash = fota_hash32(porig,php1->OriginalLength);
        if(orig_hash != php1->OriginalHash) {
            return fotaPatchOrigHash;
        }

        pstream->bst.old_start = porig;
        pstream->bst.old_end = porig + php1->OriginalLength;

    } else if(sigcmp(ppatch,(uint8_t *)FOTA_SIGNATURE_IMG1)) {
        if(patch_filled < sizeof(fota_header_fotai1_t)) {
            return fotaHeaderSmall;
        }
        pstream->undiff = false;
        fota_header_fotai1_t *phi1 = (fota_header_fotai1_t *)ppatch;
        pstream->lzgst.window_size = 1 << phi1->PackWindowOrderI1;
        if(phi1->flags != 0) {
            return fotaHeaderParsBad;
        }
    } else {
        return fotaHeaderSignatureUnsupported;
    }

//    if(pstream->undiff) {
//        //TODO: more comprehensive check here
//        if(orig_size == 0) {
//            return fotaPatchOrigWrong;
//        }
//    }

    pstream->uncompress = (pstream->lzgst.window_size > 1);

    if(pstream->uncompress) {
        pstream->lzgst.hist_buf = pram;
        pstream->lzgst.hist_buf_size = ram_size;
        if(pstream->lzgst.hist_buf_size < pstream->lzgst.window_size) {
            return fotaPackLevelBig;
        }

        pstream->lzgst.hist_buf_full = false;
        pstream->lzgst.hist_buf_pos = 0;

        pstream->lzgst.stage = ldMarkers;
        pstream->lzgst.cnt = 0;
    }

    if(!pstream->undiff) {
        *ppatch_header_size = sizeof(fota_header_fotai1_t);
        return fotaOk;
    }

    //
    pstream->bst.pold = (uint8_t *)pstream->bst.old_start;
    pstream->bst.stage = bstControl;
    pstream->bst.cnt = 0;
    pstream->bst.zero_stage = unsqIdle;

    *ppatch_header_size = sizeof(fota_header_fotap1_t);
    return fotaOk;
}
//----------------------------------------------------------------------------
fota_patch_result_t fota_patch_stream_append(fota_patch_stream_state_t *pstream, uint8_t byte) {
    if(pstream->uncompress) {
        return fotaDecodeLZG_append(pstream,byte);
    } else {
        return fota_write_decompressed(pstream,byte);
    }
}
//----------------------------------------------------------------------------
