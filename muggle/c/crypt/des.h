/******************************************************************************
 *  @file         des.h
 *  @author       Muggle Wei
 *  @email        mugglewei@gmail.com
 *  @date         2021-06-16
 *  @copyright    Copyright 2021 Muggle Wei
 *  @license      MIT License
 *  @brief        mugglec crypt DES
 *****************************************************************************/
 
#ifndef MUGGLE_C_DES_H_
#define MUGGLE_C_DES_H_

#include "muggle/c/base/macro.h"
#include "muggle/c/crypt/crypt_utils.h"

#define MUGGLE_DES_BLOCK_SIZE 8

EXTERN_C_BEGIN

typedef struct muggle_des_48bit
{
	unsigned char bytes[8];
}muggle_des_48bit_t;
typedef struct muggle_des_48bit muggle_des_subkey_t;

typedef struct muggle_des_subkeys
{
	muggle_des_subkey_t sk[16];
}muggle_des_subkeys_t;

typedef struct muggle_des_context
{
	int                  op;   //!< encryption or decryption, use MUGGLE_DECRYPT or MUGGLE_ENCRYPT
	int                  mode; //!< cipher block mode, use MUGGLE_BLOCK_CIPHER_MODE_*
	muggle_des_subkeys_t sk;   //!< DES subkeys
}muggle_des_context_t;

/**
 * @brief DES setup key schedule for mode
 *
 * @param op crypt operator
 *   - MUGGLE_DECRYPT encrypt
 *   - MUGGLE_ENCRYPT decrypt
 * @param mode block cipher mode, see MUGGLE_BLOCK_CIPHER_MODE_*
 * @param key des input key
 * @param ctx DES context
 *
 * @return
 *   - 0 success
 *   - otherwise failed, see MUGGLE_ERR_*
 */
MUGGLE_C_EXPORT
int muggle_des_set_key(
	int op,
	int mode,
	const unsigned char key[MUGGLE_DES_BLOCK_SIZE],
	muggle_des_context_t *ctx);

/**
 * @brief DES crypt with ECB mode
 *
 * @param ctx DES context
 * @param input input bytes, length must be multiple of 8
 * @param num_bytes length of input/output bytes
 * @param output output bytes
 *
 * @return
 *   - 0 success
 *   - otherwise failed, return MUGGLE_ERR_*
 */
MUGGLE_C_EXPORT
int muggle_des_ecb(
	const muggle_des_context_t *ctx,
	const unsigned char *input,
	unsigned int num_bytes,
	unsigned char *output);

/**
 * @brief DES crypt with CBC mode
 *
 * @param ctx DES context
 * @param input input bytes, length must be multiple of 8
 * @param num_bytes length of input/output bytes
 * @param iv initialization vector
 * @param output output bytes
 *
 * @return
 *   - 0 success
 *   - otherwise failed, return MUGGLE_ERR_*
 */
MUGGLE_C_EXPORT
int muggle_des_cbc(
	const muggle_des_context_t *ctx,
	const unsigned char *input,
	unsigned int num_bytes,
	unsigned char iv[MUGGLE_DES_BLOCK_SIZE],
	unsigned char *output);

/**
 * @brief DES crypt with CFB 64bit iv mode
 *
 * @param ctx DES context
 * @param input input bytes
 * @param num_bytes length of input/output bytes
 * @param iv initialization vector
 * @param iv_offset offset bytes in iv
 * @param output output bytes
 *
 * @return
 *   - 0 success
 *   - otherwise failed, return MUGGLE_ERR_*
 */
MUGGLE_C_EXPORT
int muggle_des_cfb64(
	const muggle_des_context_t *ctx,
	const unsigned char *input,
	unsigned int num_bytes,
	unsigned char iv[MUGGLE_DES_BLOCK_SIZE],
	unsigned int *iv_offset,
	unsigned char *output);

/**
 * @brief DES crypt with OFB 64bit iv mode
 *
 * @param ctx DES context
 * @param input input bytes
 * @param num_bytes length of input/output bytes
 * @param iv initialization vector
 * @param iv_offset offset bytes in iv
 * @param output output bytes
 *
 * @return
 *   - 0 success
 *   - otherwise failed, return MUGGLE_ERR_*
 */
MUGGLE_C_EXPORT
int muggle_des_ofb64(
	const muggle_des_context_t *ctx,
	const unsigned char *input,
	unsigned int num_bytes,
	unsigned char iv[MUGGLE_DES_BLOCK_SIZE],
	unsigned int *iv_offset,
	unsigned char *output);

/**
 * @brief DES crypt with CTR mode
 *
 * @param ctx DES context
 * @param input input bytes
 * @param num_bytes length of input/output bytes
 * @param nonce
 * @param nonce_offset offset bytes in nonce
 * @param output output bytes
 * @param stream_block ciphertext of nonce
 *
 * @return
 *   - 0 success
 *   - otherwise failed, return MUGGLE_ERR_*
 */
MUGGLE_C_EXPORT
int muggle_des_ctr(
	const muggle_des_context_t *ctx,
	const unsigned char *input,
	unsigned int num_bytes,
	uint64_t *nonce,
	unsigned int *nonce_offset,
	unsigned char stream_block[MUGGLE_DES_BLOCK_SIZE],
	unsigned char *output);

EXTERN_C_END

#endif
