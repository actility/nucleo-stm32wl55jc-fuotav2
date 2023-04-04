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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fota_misc.h"

//----------------------------------------------------------------------------
int packed_name(char *fname_source, char *fname_target, char *sep,
               char *buf, size_t bufsize) {
#define BUFADD(_s,_slen)  \
    if(bufsize < (size_t)_slen) { return 0; }\
    memcpy(buf,_s,_slen);\
    buf += _slen;\
    bufsize -= _slen;
//

    char *src_path_end = strrchr(fname_source,'/');
    if(!src_path_end) { src_path_end = strrchr(fname_source,'\\'); }
    char *tgt_path_end = strrchr(fname_target,'/');
    if(!tgt_path_end) { tgt_path_end = strrchr(fname_target,'\\'); }

#if 0 // out to working path
    if(src_path_end) {
        int path_len = src_path_end - fname_source + 1;
        BUFADD(fname_source,path_len);
    }
#endif

    char *sname = src_path_end ? src_path_end + 1 : fname_source;
    char *tname = tgt_path_end ? tgt_path_end + 1 : fname_target;

    char *sdot = strrchr(sname,'.');
    if(!sdot) { sdot = &sname[strlen(sname)]; }
    char *tdot = strrchr(tname,'.');
    if(!tdot) { tdot = &tname[strlen(tname)]; }

    int snlen = sdot - sname;
    int tnlen = tdot - tname;


    BUFADD(sname,snlen);

    int sepl = strlen(sep);
    BUFADD(sep,sepl);

    BUFADD(tname,tnlen);

    int extl = strlen(tdot);
    BUFADD(tdot,extl);

    BUFADD("\0",1);
    return 1;
}
//----------------------------------------------------------------------------
int target_name(char *fname_packed, char *buf, size_t bufsize) {
    char *src_path_end = strrchr(fname_packed,'/');
    if(!src_path_end) { src_path_end = strrchr(fname_packed,'\\'); }

#if 0 // out to working path
    if(src_path_end) {
        int path_len = src_path_end - fname_packed + 1;
        BUFADD(fname_packed,path_len);
    }
#endif

    char *sname = src_path_end ? src_path_end + 1 : fname_packed;

    char *sdot = strrchr(sname,'.');
    if(!sdot) { sdot = &sname[strlen(sname)]; }

    const char tname[] = "target";
    BUFADD(tname,sizeof(tname) - 1);

    int extl = strlen(sdot);
    BUFADD(sdot,extl);

    BUFADD("\0",1);
    return 1;
}
//----------------------------------------------------------------------------
void bbuf_alloc(bbuf_t *bb, size_t size) {
    bb->size = size;
    bb->data = malloc(bb->size);
    bb->pos = 0;
}
//----------------------------------------------------------------------------
void bbuf_free(bbuf_t *bb) {
    if(bb->data) { free(bb->data); bb->data = NULL; }
}
//----------------------------------------------------------------------------
bool bbuf_fread(bbuf_t *bb, char *fname) {
    bbuf_free(bb);
    FILE *fp = fopen(fname,"rb");
    if(!fp) {
        printf("  ERROR opening %s\n",fname);
        return false;
    }
    fseek(fp,0,SEEK_END);
    bbuf_alloc(bb,ftell(fp));
    fseek(fp,0,SEEK_SET);
    fread(bb->data,1,bb->size,fp);
    fclose(fp);
    return true;
}
//----------------------------------------------------------------------------
bool bbuf_fwrite(bbuf_t *bb, char *fname) {
    FILE *fpd = fopen(fname,"wb");
    if(!fpd) {
        printf("  ERROR opening %s\n",fname);
        return false;
    }
    int written = fwrite(bb->data,1,bb->pos,fpd);
    fclose(fpd);
    if(written != bb->pos)  {
        printf("  failed write to %s\n",fname);
        return false;
    }
    return true;
}
//----------------------------------------------------------------------------
int diff_squeeze(uint8_t *src, size_t src_size, uint8_t *dst, size_t dst_size) {
    // replace zeros by [0,N]
    uint8_t *src_end = src + src_size;
    uint8_t *dst_end = dst + dst_size;
    uint8_t *dst_start = dst;
    for(; src < src_end && dst < dst_end; ++src, ++dst) {
        if((*dst = *src) != 0) { continue; }
        int rep = 0;
        while(++src < src_end) {
            if(*src == 0) {
                if(++rep >= 0x7FFF) { break; }
            }
            else { --src; break;}
        }
        if(++dst >= dst_end) { return -2; }
        if(rep <= 0x7F) {
            *dst = rep;
        } else {
            if(dst + 1 >= dst_end) { return -3; }
            *dst++ = (uint8_t)((rep >> 8) | 0x80);
            *dst = (uint8_t)rep;
        }
    }
    if(src < src_end && dst >= dst_end) { return -1; }
    return dst - dst_start;
}
//----------------------------------------------------------------------------
int diff_unsqueeze(uint8_t *src, size_t src_size, uint8_t *dst, size_t dst_size) {
    // restore zeros from [0,N]
    uint8_t *src_end = src + src_size;
    uint8_t *dst_end = dst + dst_size;
    uint8_t *dst_start = dst;
    for(; src < src_end && dst < dst_end; ++src, ++dst) {
        if((*dst = *src) != 0) { continue; }
        if(++src >= src_end) { return -3; }
        int zcnt = *src;
        if(zcnt & 0x80) {
            if(++src >= src_end) { return -4; }
            zcnt = ((zcnt & ~0x80) << 8) | *src;
        }
        if(dst + zcnt >= dst_end) { return -2; }
        while(zcnt-- > 0) {
            *++dst = 0;
        }
    }
    return dst - dst_start;
}
//----------------------------------------------------------------------------
char *fota_patch_result_str(fota_patch_result_t code) {
    switch(code) {
#define SK(_fr) case _fr: return #_fr
    SK(fotaOk);
    SK(fotaHeaderSignatureSmall);
    SK(fotaHeaderSmall);
    SK(fotaHeaderSignatureUnsupported);
    SK(fotaHeaderParsBad);
    SK(fotaPatchOrigHash);
    SK(fotaPatchOrigSmall);
    SK(fotaPackLevelBig);
    SK(fotaLzgMiss);
    SK(fotaBspatchOrigMiss);
    SK(fotaTargetOverflow);
    SK(fotaTargetWriteFailed);
    SK(fotaError);
#undef SK
    }
    return "Unknown";
}
//----------------------------------------------------------------------------
