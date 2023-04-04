/*  _        _   _ _ _ _
   / \   ___| |_(_) (_) |_ _   _
  / _ \ / __| __| | | | __| | | |
 / ___ \ (__| |_| | | | |_| |_| |
/_/   \_\___|\__|_|_|_|\__|\__, |
                           |___/
    (C)2017 Actility
License: see LICENCE_SLA0ACT.TXT file include in the project
Description: Unpacker main routine
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fota_patch.h"
#include "fota_misc.h"
//----------------------------------------------------------------------------
#define PATCH_NAME_SEP "_PATCH_"

bbuf_t src, patch, diff, out, /*lzg_sqzd,*/ diff_sqzd, diff_unsqzd, out_sqzd, cmp;
//----------------------------------------------------------------------------
void usage(void) {
    printf("  Usage: fota-unpack packedfile [targetfile] [-p patchedfile] [-m matchfile]\n");
    // [-t]
}
//----------------------------------------------------------------------------
unsigned int write_tgt_pos;
// *fota_patch_write_target_fun_t
fota_patch_result_t fota_write_target( uint8_t b) {
    if(write_tgt_pos >= out.size) {
        return fotaTargetOverflow;
    }
    out.data[write_tgt_pos++] = b;
    return fotaOk;
}
//----------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    printf("FOTA Unpack tool v." GIT_DESCRIPTION "\n");

    bool debug = false;
    char *fname_orig = NULL;
    char *fname_test = NULL;
    char *sarg[2];
    unsigned int sargn = 0;
    memset(sarg, 0,sizeof(sarg));
    enum { oNone, oDiffOrig, oMatch} option = oNone;
    for(int i = 1; i < argc; ++i) {
        //+++ bypass extra CR (happened on Linux)
        if(argv[i][strlen(argv[i]) - 1] == '\r') {
            argv[i][strlen(argv[i]) - 1] = '\0';
        }
        if(argv[i][0] == '\0') {
            continue;
        }
        //+++

        if(argv[i][0] == '-' && option == oNone) {
            if(strlen(argv[i]) == 2) {
                switch(argv[i][1]) {
                case 'p': option = oDiffOrig; continue;
                case 't': debug = true; continue;
                case 'm': option = oMatch; continue;
                }
                fprintf(stderr,"  unknown option '%s'\n",argv[i]);
                return 1;
            } else {
                fprintf(stderr,"  invalid option '%s'\n",argv[i]);
                return 1;
            }
        }

        switch(option) {
        case oNone:
            if(sargn >= sizeof(sarg)/sizeof(*sarg)) {
                fprintf(stderr,"  too many arguments\n");
                return 1;
            }
            sarg[sargn++] = argv[i];
            continue;
        case oDiffOrig:
            fname_orig = argv[i];
            break;
        case oMatch:
            fname_test = argv[i];
            break;
        }
        option = oNone;
    }

    if(sargn < 1) { usage(); return 1; }
    char *fname_packed = sarg[0];
    if(sargn > 2) { usage(); return 1; }
    char fname_tgt_buf[strlen(fname_packed) + 1];
    char *fname_target = fname_tgt_buf;
    if(sargn == 2) {
        fname_target = sarg[1];
    } else if(!target_name(fname_packed,fname_tgt_buf,sizeof(fname_tgt_buf))) {
        fprintf(stderr,"  error target_name()\n");
        return 1;
    }

    if(fname_orig) {
        printf("  Apply patch\n         %s\n    from %s\n      to %s\n",
               fname_packed,fname_orig,fname_target);
    } else {
        printf("  Uncompress\n         %s\n      to %s\n",
               fname_packed,fname_target);
    }

    int ret = 1;
    do{
        if(!bbuf_fread(&patch,fname_packed)) { break; }

        if(fname_orig) {
            if(!bbuf_fread(&src,fname_orig)) { break; }
        }

        //+++
        bbuf_alloc(&diff_sqzd, 1 << 20 /*ph->UnpackedSize*/);
        bbuf_alloc(&diff, 1 << 20/*ph->DiffSize*/);
        bbuf_alloc(&out, 1 << 20 /*ph->TargetSize*/);
        //+++
        uint8_t ram[1 << 15];
        write_tgt_pos = 0;
        // erase FLASH here

        fota_patch_result_t fres = fota_patch(patch.data,patch.size,
                                              ram,sizeof(ram),
                                              src.data,src.size);
        if(fres == fotaOk) {
            // commit last FLASH write here
            out.pos = write_tgt_pos;
        } else {
            fprintf(stderr,"  failed fota_patch(), error %d (%s)\n",fres,fota_patch_result_str(fres));
        }

//        if((size_t)out.pos != ph->TargetSize) {
//            printf("  failed TargetSize\n");
//            break;
//        }

        if(!bbuf_fwrite(&out,fname_target)) { break; }

        if(debug) {
            if(!bbuf_fwrite(&diff_sqzd,"out_diff_sqzd.bin")) { break; }
            if(!bbuf_fwrite(&diff,"out_diff.bin")) { break; }

            if(diff.pos > 0) {
                bbuf_alloc(&diff_unsqzd, 1 << 20/*ph->DiffSize*/);
                diff_unsqzd.pos = diff_unsqueeze(diff_sqzd.data,diff_sqzd.pos,
                                                 diff_unsqzd.data,diff_unsqzd.size);
                if(diff_unsqzd.pos < 0) {
                    fprintf(stderr,"  failed diff_unsqueeze() %d\n",diff_unsqzd.pos);
                    break;
                }
                if(!bbuf_fwrite(&diff_unsqzd,"out_diff_unsqzd.bin")) { break; }
            }
        }


        printf("  Done\n");
        ret = 0;
    }while(0);

    if(!ret && fname_test && *fname_test) {
        if(bbuf_fread(&cmp,fname_test)) {
            bool match = ((int)cmp.size == out.pos
                          && memcmp(out.data,cmp.data,out.pos) == 0);
                printf("  Target %sMATCH with %s\n",
                       match ? "" : "MIS",fname_test);
                
                if(!match) ret = 2;
        }
    }


    bbuf_free(&src);
    bbuf_free(&patch);
    bbuf_free(&diff);
    bbuf_free(&out);
//    bbuf_free(&lzg_sqzd);
    bbuf_free(&diff_sqzd);
    bbuf_free(&diff_unsqzd);
    bbuf_free(&out_sqzd);

    return ret;
}
//----------------------------------------------------------------------------
void debug_write_decompressed(uint8_t b) {
    if(diff_sqzd.pos >= (int)diff_sqzd.size) {
        return;
    }
    diff_sqzd.data[diff_sqzd.pos++] = b;
}
//----------------------------------------------------------------------------
void debug_write_unsqueezed(uint8_t b) {
    if(diff.pos >= (int)diff.size) {
        return;
    }
    diff.data[diff.pos++] = b;
}
//----------------------------------------------------------------------------
