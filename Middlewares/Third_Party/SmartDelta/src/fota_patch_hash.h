/*  _        _   _ _ _ _
   / \   ___| |_(_) (_) |_ _   _
  / _ \ / __| __| | | | __| | | |
 / ___ \ (__| |_| | | | |_| |_| |
/_/   \_\___|\__|_|_|_|\__|\__, |
                           |___/
    (C)2017 Actility
License: see LICENCE_SLA0ACT.TXT file include in the project
Description: HASH calculators
*/

#ifndef FOTA_PATCH_HASH_H
#define FOTA_PATCH_HASH_H
//----------------------------------------------------------------------------
//#include <stddef.h>
#include <stdint.h>

//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//----------------------------------------------------------------------------
uint32_t fota_hash32(uint8_t *data, int length);
uint32_t fota_hash32_ether_accum(uint32_t crc32, uint8_t *data, int length);
uint32_t fota_hash32_append(uint32_t crc, uint8_t b);
//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------
#endif // FOTA_PATCH_HASH_H
