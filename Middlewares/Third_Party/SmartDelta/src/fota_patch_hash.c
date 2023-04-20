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

#include "fota_patch_hash.h"

//----------------------------------------------------------------------------
//static const uint32_t ethernet_polynomial = 0x04c11db7U;
static const uint32_t ethernet_polynomial_rev = 0xEDB88320U;

//----------------------------------------------------------------------------
uint32_t fota_hash32_append(uint32_t crc, uint8_t b) {
    for(int bit = 0; bit < 8; bit++, b >>= 1) {
        if((crc ^ b) & 1) {
            crc = (crc >> 1) ^ ethernet_polynomial_rev;
        } else {
            crc >>= 1;
        }
    }
    return crc;
}
//----------------------------------------------------------------------------
uint32_t fota_hash32_ether_accum(uint32_t crc32, uint8_t *data, int length) { // =cyg_ether_crc32_accumulate()
    crc32 = crc32^0xffffffff;
    while(--length >= 0) {
      crc32 = fota_hash32_append(crc32, *data++);
    }
    return crc32^0xffffffff;
}
//----------------------------------------------------------------------------
uint32_t fota_hash32(uint8_t *data, int length) { // =cyg_crc32()
    uint32_t crc = 0;
    while(--length >= 0) {
        crc = fota_hash32_append(crc,*data++);
    }
    return crc;
}
//----------------------------------------------------------------------------
