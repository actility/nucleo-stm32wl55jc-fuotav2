/*  _        _   _ _ _ _
   / \   ___| |_(_) (_) |_ _   _
  / _ \ / __| __| | | | __| | | |
 / ___ \ (__| |_| | | | |_| |_| |
/_/   \_\___|\__|_|_|_|\__|\__, |
                           |___/
    (C)2017 Actility
License: see LICENCE_SLA0ACT.TXT file include in the project
Description: Packer main routine
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "fota_patch_hash.h"
#include "fota_misc.h"
#include "Cheedoong-bsdiff/bsdiff.h"
#include "lzg_e/lzg.h"
//----------------------------------------------------------------------------
#define PATCH_NAME_SEP      "_PATCH_"
#define COMPRESSED_NAME_SEP "PACKED_"

bbuf_t src, tgt, diff, diff_sqzd, diff_unsqzd, lzg, patch;
//----------------------------------------------------------------------------
void usage(void) {
    printf("  Usage: fota-pack file [packedfile] [-p patchedfile] [-l <LEVEL>]\n");
    // [-t] [-z]
}
//----------------------------------------------------------------------------
int bs_write(struct bsdiff_stream* stream, const void* buffer, int size) {
    bbuf_t *ppatch = (bbuf_t *)stream->opaque;

    int n = 0;
    int room;
    while((room = ppatch->size - ppatch->pos) < size) {
        if(++n > 3) { return -1; }
        ppatch->size *= 2;
        ppatch->data = realloc(ppatch->data,ppatch->size);
        room = ppatch->size - ppatch->pos;
    }

    memcpy(&ppatch->data[ppatch->pos],buffer,size);
    ppatch->pos += size;
    return 0;
}
//----------------------------------------------------------------------------
int bbuf_compress(bbuf_t *in, bbuf_t *out, int window_ord) {

    // lzg compress

    // Determine maximum size of compressed data
    lzg_uint32_t maxEncSize = LZG_MaxEncodedSize(in->pos);
    // Allocate memory for the compressed data
    bbuf_alloc(out,maxEncSize);

    unsigned char *encBuf = out->data;
    // Compress
    lzg_encoder_config_t cfg;
    LZG_InitEncoderConfig(&cfg);
    //cfg.level=LZG_LEVEL_9;
    cfg.window_order = window_ord;
    int encSize = LZG_Encode(in->data,
                             in->pos,
                             encBuf, maxEncSize, &cfg);
    if( !encSize ) {
        fprintf(stderr,"  failed LZG_Encode()");
        return -1;
    } else if(encSize < 0)
        return 1;
    //
    out->pos = encSize;
    return 0;
}
//----------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    printf("FOTA Pack tool v." GIT_DESCRIPTION "\n");

    bool debug = false;
//    ...bool patch = false;
    int compress_window_order = 11;
    bool zero_squeeze = true;
    char *fname_source = "";
    char *sarg[2];
    unsigned int sargn = 0;
    memset(sarg, 0,sizeof(sarg));
    enum { oNone, oWindowOrder, oDiffOrig} option = oNone;
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
                case 'l': option = oWindowOrder; continue;
                case 't': debug = true; continue;
                case 'p': option = oDiffOrig; continue;
                case 'z': zero_squeeze = false; continue;
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
        case oWindowOrder:
            compress_window_order = atoi(argv[i]);
            break;
        case oDiffOrig:
            fname_source = argv[i];
            break;
        }
        option = oNone;
    }
    if(compress_window_order < 4 || compress_window_order > PACK_WINDOW_ORDER_MAX) {
        fprintf (stderr,"  LEVEL wrong (4..30)\n");
        return 1;
    }

    if(sargn < 1) { usage(); return 1; }
    char *fname_target = sarg[0];
    if(sargn > 2) { usage(); return 1; }
    char fname_packed_buf[strlen(fname_source) + strlen(fname_target) + sizeof(PATCH_NAME_SEP) + 1];
    char *fname_packed = fname_packed_buf;
    if(sargn == 2) {
        fname_packed = sarg[1];
    } else if(!packed_name(fname_source,fname_target,
                          *fname_source == '\0' ? COMPRESSED_NAME_SEP : PATCH_NAME_SEP,
                          fname_packed_buf,sizeof(fname_packed_buf))) {
        fprintf(stderr,"  error patch_name()\n");
        return 1;
    }

    if(*fname_source != '\0') {
    printf("  Create patch\n    from %s\n      to %s\n      as %s\n",
           fname_source,fname_target,fname_packed);
    } else {
        printf("  Create compressed\n         %s\n      as %s\n",
               fname_target,fname_packed);
    }

    int ret = 1;
    do{
        if(!bbuf_fread(&tgt,fname_target)) { break; }
        tgt.pos = tgt.size;
        bbuf_t *pbbuf_to_compress = &tgt;

        if(*fname_source != '\0') {
            if(!bbuf_fread(&src,fname_source)) { break; }

            size_t sz = src.size > tgt.size ? src.size : tgt.size;
            sz *= 2;
            bbuf_alloc(&diff,sz);

            struct bsdiff_stream stream;
            stream.malloc = malloc;
            stream.free = free;
            stream.write = bs_write;
            stream.opaque = &diff;
            if(bsdiff(src.data,src.size,tgt.data,tgt.size,&stream) != 0) {
                fprintf(stderr,"  failed bsdiff()");
                break;
            }
            if(debug) {
                bbuf_fwrite(&diff,"in_diff.bin");
            }

#if 0
            int diff_stat[256];
            memset(diff_stat,0,sizeof(diff_stat));
            for(uint8_t *p = diff.data + diff.pos; p >= diff.data; --p) {
                ++diff_stat[*p];
            }
#define FILLMAX (100)
            char fill[FILLMAX + 1];
            memset(fill,'.',FILLMAX);
            fill[FILLMAX] = '\0';
            for(int i=0; i < 256; ++i) {
                int n = FILLMAX - diff_stat[i]/100;
                char ovf = ' ';
                if(n < 0) { n = 0; ovf = '+'; }
                printf("  val %3d: %8d %s%c\n",i,diff_stat[i],&fill[n],ovf);
            }
#endif

            //
            bbuf_alloc(&diff_sqzd,diff.size);
            diff_sqzd.pos = diff_squeeze(diff.data,diff.pos,
                                         diff_sqzd.data,diff_sqzd.size);
            if(diff_sqzd.pos < 0) {
                fprintf(stderr,"  diff_squeeze() err %d\n",diff_sqzd.pos);
                break;
            } else {
                if(debug) {
                    if(!bbuf_fwrite(&diff_sqzd,"in_diff_sqzd.bin")) { break; }
                }
            }
            if(zero_squeeze) {
                pbbuf_to_compress = &diff_sqzd;
            } else {
                pbbuf_to_compress = &diff;
            }
        }
        //

        printf("  Compressing...\n");
        printf("  LEVEL=%d (History window %u bytes)\n",
               compress_window_order,1U << compress_window_order);
        int bres = bbuf_compress(pbbuf_to_compress,&lzg,compress_window_order);
        if(bres < 0) { break; }
        if(bres > 0) {
            printf("  Size overflow, falling back to copy\n");
            if((int)lzg.size < pbbuf_to_compress->pos) { break; }
            memcpy(lzg.data,pbbuf_to_compress->data,pbbuf_to_compress->pos);
            lzg.pos = pbbuf_to_compress->pos;
            compress_window_order = 0;// no compression mark
        }
        if(debug) {
            if(!bbuf_fwrite(&lzg,"lzg.bin")) { break; }
        }

        bbuf_alloc(&patch,sizeof(fota_header_fotap1_t) + lzg.pos);
        if(pbbuf_to_compress == &tgt) {
            fota_header_fotai1_t *ph = (fota_header_fotai1_t *)patch.data;
            strncpy(ph->Signature,FOTA_SIGNATURE_IMG1,sizeof(ph->Signature));
            ph->PackWindowOrderI1 = compress_window_order;
            ph->flags = 0;
            memcpy(ph + 1,lzg.data,lzg.pos);
            patch.pos = sizeof(*ph) + lzg.pos;
        } else {
            fota_header_fotap1_t *ph = (fota_header_fotap1_t *)patch.data;
            strncpy(ph->Signature,FOTA_SIGNATURE_PATCH1,sizeof(ph->Signature));
            ph->PackWindowOrderP1 = compress_window_order;
            ph->ZeroNotSqueezed = !zero_squeeze;
            ph->flags2=0;
            ph->OriginalLength = src.size;
            ph->OriginalHash = fota_hash32(src.data,src.size);
            memcpy(ph + 1,lzg.data,lzg.pos);
            patch.pos = sizeof(*ph) + lzg.pos;
            printf("  Original: length %u hash %08X\n",
                   ph->OriginalLength,ph->OriginalHash);
        }
        if(!bbuf_fwrite(&patch,fname_packed)) { break; }

        printf("  Done\n");
        ret = 0;
    }while(0);


    bbuf_free(&src);
    bbuf_free(&tgt);
    bbuf_free(&diff);
    bbuf_free(&diff_sqzd);
    bbuf_free(&diff_unsqzd);
    bbuf_free(&lzg);
    bbuf_free(&patch);
    return ret;
}
//----------------------------------------------------------------------------
