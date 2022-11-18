/*  _        _   _ _ _ _
   / \   ___| |_(_) (_) |_ _   _
  / _ \ / __| __| | | | __| | | |
 / ___ \ (__| |_| | | | |_| |_| |
/_/   \_\___|\__|_|_|_|\__|\__, |
                           |___/
    (C)2017 Actility
License: see LICENCE_SLA0ACT.TXT file include in the project
Description: Smart Delta RMC server ECDSA signature verification
*/
/*
 * IMPORTANT NOTE: Current implementation do not use security features of
 * STM32Lx processors and insecure if attacked in object form.
 * In the future could be re-implemented to use STM32Lx hardware
 * security features both to protect public key and crypto functions
 * from tampering.
 */
/**
  ******************************************************************************
  * @author  MCD Application Team
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include 	<stdint.h>
#include 	<stddef.h>
#include 	<string.h>
#include 	"verify_signature.h"
#include 	"mbedtls/ecp.h"
#include	"mbedtls/ecdsa.h"
#include 	"mbedtls/asn1write.h"
#include 	"mbedtls/sha256.h"

#define SMARTDELTA_PUBLIC_KEY 	{  \
  0xde, 0xde, 0x0b, 0x9a, 0xf1, 0x03, 0xf1, 0xa4, 0x76, 0x73, 0xd9, 0xa0, 0x06, 0x16, 0x37, 0x7c, \
  0xba, 0xf6, 0x81, 0x12, 0x0a, 0x8f, 0x4e, 0x7f, 0xce, 0xac, 0xe4, 0x62, 0x7c, 0xbf, 0xac, 0xfd, \
  0x82, 0x1d, 0x2b, 0x47, 0xfb, 0x14, 0x2a, 0xb2, 0x6e, 0xb7, 0x87, 0xb2, 0x55, 0xeb, 0xb7, 0x13, \
  0x6a, 0x6a, 0xdc, 0xbf, 0x39, 0x08, 0xe7, 0xed, 0x0b, 0x49, 0xb1, 0x41, 0x25, 0xca, 0x03, 0x05  \
}


/*
 * Read and check signature
 * We always pad signature to 72 bytes so ASN.1 length check will fail if
 * actual signature is shorter. So we eliminate ASN.1 length check
 * in contrast to original mbedTLS implementation
 */
static int mbedtls_ecdsa_read_signature_pad( mbedtls_ecdsa_context *ctx,
                          const unsigned char *hash, size_t hlen,
                          const unsigned char *sig, size_t slen )
{
	int ret;
    unsigned char *p = (unsigned char *) sig;
    const unsigned char *end = sig + slen;
    size_t len;
    mbedtls_mpi r, s;

    mbedtls_mpi_init( &r );
    mbedtls_mpi_init( &s );

    if( ( ret = mbedtls_asn1_get_tag( &p, end, &len,
                    MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 )
    {
        ret += MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
        goto cleanup;
    }

    if( ( ret = mbedtls_asn1_get_mpi( &p, end, &r ) ) != 0 ||
        ( ret = mbedtls_asn1_get_mpi( &p, end, &s ) ) != 0 )
    {
        ret += MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
        goto cleanup;
    }

    ret = mbedtls_ecdsa_verify( &ctx->grp, hash, hlen, &ctx->Q, &r, &s );

cleanup:
    mbedtls_mpi_free( &r );
    mbedtls_mpi_free( &s );

    return( ret );
}

/**
  * @brief  SHA256 HASH digest compute example.
  * @param  InputMessage: pointer to input message to be hashed.
  * @param  InputMessageLength: input data message length in byte.
  * @param  MessageDigest: pointer to output parameter that will handle message digest
  * @param  MessageDigestLength: pointer to output digest length.
  * @retval error status: 0 if success
  */
static int32_t SmartDelta_SHA256_HASH_DigestCompute(const uint8_t *InputMessage, const int32_t InputMessageLength,
                                                   uint8_t *MessageDigest, int32_t *MessageDigestLength)
{
  int32_t ret;
  int32_t error_status = -1;

  /*
   * InputMessageLength is never negative (see SE_CRYPTO_Authenticate_Metadata)
   * so the cast to size_t is valid
   */

  /* init of a local sha256 context */
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);

  ret = mbedtls_sha256_starts_ret(&ctx, 0);   /* 0 for sha256 */

  if (0 == ret)
  {
    ret = mbedtls_sha256_update_ret(&ctx, InputMessage, (size_t)InputMessageLength); /* casting is fine because size_t
                                                                                        is unsigned int in ARM C */

    if (0 == ret)
    {
      ret = mbedtls_sha256_finish_ret(&ctx, MessageDigest);

      if (0 == ret)
      {
        error_status = 0;
        *MessageDigestLength = 32; /* sha256 */
      }
      else
      {
        *MessageDigestLength = 0;
      }
    }
  }

  mbedtls_sha256_free(&ctx);

  return error_status;
}

/**
  * @brief Smart Delta patch verify signature function.
  *        Authenticates the Smart Delta patch with header.
  * @param Patch : Patch body with header plus signature.
  * @param PatchSize : Size of Patch
  * @retval SMARTDELTA_OK if successful, SMARTDELTA_ERROR otherwise.
  */
int32_t fota_patch_verify_signature (uint8_t *Patch, size_t PatchSize)
{
  int32_t e_ret_status = SMARTDELTA_ERROR;
  int32_t ret; /* mbedTLS return code */

/*
 * Variable used to store the Asymmetric Key.
 *
 * In secured implementation it should be stored in protected SRAM1 area.
 * (even if this is a public key it should be stored like the secret key).
 */
static uint8_t m_aSmartDelta_PubKey[SMARTDELTA_ASYM_PUBKEY_LEN] = SMARTDELTA_PUBLIC_KEY;

   /*
	* Local variables for authentication procedure.
	*/
	int32_t status = 0;
	mbedtls_ecdsa_context ctx;
	/* Firmware metadata to be authenticated and reference MAC */
	uint8_t *pPayload;    /* Metadata payload */
	int32_t payloadSize;        /* Metadata length to be considered for hash */
	uint8_t *pSign;             /* Reference MAC (ECCDSA signed SHA256 of the FW metadata) */
	uint8_t MessageDigest[32];      /* The message digest is a sha256 so 32 bytes */
	int32_t MessageDigestLength = 0;


  /*
   * Key to be used for crypto operations
   *
   * In secured implementation this is a pointer to m_aSmartDelta_PubKey and it can be a
   * local variable, the pointed data is protected
   */
  uint8_t *pKey;

  if (NULL == Patch)
  {
    return SMARTDELTA_ERROR;
  }

  if (PatchSize <= SMARTDELTA_MAC_LEN)
  {
	return SMARTDELTA_ERROR;
  }
  /* Retrieve the ECC Public Key */
  /* SmartDelta_ReadKey_Pub(&(m_aSmartDelta_PubKey[0])); */
  pKey = &(m_aSmartDelta_PubKey[0]);

  /* Set the local variables required to handle the Smart Delta signature during the authentication procedure */
  pPayload = Patch;
  payloadSize = PatchSize - SMARTDELTA_MAC_LEN;
  pSign = pPayload + payloadSize;

  /* Compute the SHA256 of the Smart Delta patch and header */
  status = SmartDelta_SHA256_HASH_DigestCompute(pPayload,
                                               payloadSize,
                                               (uint8_t *)MessageDigest,
                                               &MessageDigestLength);

  if (0 == status)
  {
    /* mbedTLS resources */
	mbedtls_ecp_group_init(&ctx.grp);
    mbedtls_ecp_point_init(&ctx.Q);

    ret = mbedtls_ecp_group_load(&ctx.grp, MBEDTLS_ECP_DP_SECP256R1);

    if (ret != 0)
    {
      e_ret_status = SMARTDELTA_ERROR;
    }
    else
    {
      /* We must use the mbedTLS format for the public key */
      unsigned char aKey[65];
      aKey[0] = 0x04; /* uncompressed point */
      (void)memcpy(&aKey[1], pKey, 64);

      /* Public key loaded as point Q */
      ret = mbedtls_ecp_point_read_binary(&ctx.grp, &ctx.Q, aKey, 65);

      if (ret != 0)
      {
        e_ret_status = SMARTDELTA_ERROR;
      }
      else
      {
        /*
         * Read signature ASN.1 DER encoded and verify
         */
    	ret = mbedtls_ecdsa_read_signature_pad( &ctx, (uint8_t *)MessageDigest,
    			MessageDigestLength, pSign, SMARTDELTA_MAC_LEN );

        if (ret != 0)
        {
          e_ret_status = SMARTDELTA_ERROR;
        }
        else
        {
          e_ret_status = SMARTDELTA_OK;
        }
      }

      /* clean-up the mbedTLS resources */
      mbedtls_ecp_point_free(&ctx.Q);
      mbedtls_ecp_group_free(&ctx.grp);
    }
  } /* else the status is already SE_ERROR */

  /* Clean-up the ECC public key in RAM */
  /* SE_CLEAN_UP_PUB_KEY(); */

  /* Return status*/
  return e_ret_status;
}
