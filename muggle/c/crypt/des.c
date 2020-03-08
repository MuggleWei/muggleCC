/*
 *	author: muggle wei <mugglewei@gmail.com>
 *
 *	Use of this source code is governed by the MIT license that can be
 *	found in the LICENSE file.
 */

#include "des.h"
#include <stddef.h>
#include "muggle/c/base/utils.h"
#include "muggle/c/log/log.h"
#include "openssl/openssl_des.h"

// default callbacks
int des_default_ip(muggle_64bit_block_t *block)
{
	MUGGLE_OPENSSL_DES_IP(block->u32.l, block->u32.h);
	return 0;
}
int des_default_fp(muggle_64bit_block_t *block)
{
	MUGGLE_OPENSSL_DES_FP(block->u32.l, block->u32.h);
	return 0;
}
int des_default_gen_subkey(const muggle_64bit_block_t *key, muggle_des_subkeys_t *ks)
{
	muggle_openssl_des_set_key_unchecked((unsigned char*)(&key->bytes[0]), ks);
	return 0;
}
muggle_64bit_block_t des_default_ep(uint32_t r)
{
	muggle_64bit_block_t block;
	block.u32.l = r;
	block.u32.h = r;
	return block;
}
uint32_t des_default_sbox(muggle_64bit_block_t v)
{
	return muggle_openssl_des_sbox(v);
}

muggle_des_cb_t des_default_encrypt_callbacks = {
	des_default_ip,
	des_default_fp,
	des_default_gen_subkey,
	des_default_ep,
	des_default_sbox
};
muggle_des_cb_t des_default_decrypt_callbacks = {
	des_default_fp,
	des_default_ip,
	des_default_gen_subkey,
	des_default_ep,
	des_default_sbox
};

typedef int (*fn_muggle_des_cipher)(
	int enc,
	const unsigned char *input,
	unsigned char *output,
	unsigned int num_bytes,
	muggle_des_subkeys_t *ks,
	muggle_64bit_block_t *iv,
	int update_iv,
	muggle_des_cb_t *callbacks);


// callbacks
int muggle_des_ecb(
	int enc, const unsigned char *input, unsigned char *output, unsigned int num_bytes,
	muggle_des_subkeys_t *ks, muggle_64bit_block_t *iv,	int update_iv, muggle_des_cb_t *callbacks);
int muggle_des_cbc(
	int enc, const unsigned char *input, unsigned char *output, unsigned int num_bytes,
	muggle_des_subkeys_t *ks, muggle_64bit_block_t *iv,	int update_iv, muggle_des_cb_t *callbacks);
int muggle_des_cfb(
	int enc, const unsigned char *input, unsigned char *output, unsigned int num_bytes,
	muggle_des_subkeys_t *ks, muggle_64bit_block_t *iv,	int update_iv, muggle_des_cb_t *callbacks);
int muggle_des_ofb(
	int enc, const unsigned char *input, unsigned char *output, unsigned int num_bytes,
	muggle_des_subkeys_t *ks, muggle_64bit_block_t *iv,	int update_iv, muggle_des_cb_t *callbacks);
int muggle_des_ctr(
	int enc, const unsigned char *input, unsigned char *output, unsigned int num_bytes,
	muggle_des_subkeys_t *ks, muggle_64bit_block_t *iv,	int update_iv, muggle_des_cb_t *callbacks);

fn_muggle_des_cipher s_fn_muggle_des[] = {
	muggle_des_ecb,
	muggle_des_cbc,
	muggle_des_cfb,
	muggle_des_ofb,
	muggle_des_ctr,
};

int muggle_des_crypt(
	int enc,
	const muggle_64bit_block_t *input,
	muggle_64bit_block_t *output,
	muggle_des_subkeys_t *ks,
	muggle_des_cb_t *callbacks)
{
	if (callbacks == NULL)
	{
		if (enc)
		{
			callbacks = &des_default_encrypt_callbacks;
		}
		else
		{
			callbacks = &des_default_decrypt_callbacks;
		}
	}

	output->u64 = input->u64;

	uint32_t s, tmp;
	muggle_64bit_block_t t;
	if (enc)
	{
		// IP
		if (callbacks->ip(output) != 0)
		{
			MUGGLE_LOG_ERROR("DES crypt failed, failed IP");
			return -1;
		}

		// Feistel, 16 rounds
		for (int i = 0; i < 16; ++i)
		{
			tmp = output->u32.l;
			output->u32.l = output->u32.h;
			output->u32.h = tmp;

			t = callbacks->ep(output->u32.h);
			t.u64 ^= ks->subkey[i].u64;
			s = callbacks->sbox(t);
			output->u32.l ^= s;
		}

		// FP
		if (callbacks->fp(output) != 0)
		{
			MUGGLE_LOG_ERROR("DES crypt failed, failed FP");
			return -1;
		}
	}
	else
	{
		// FP
		if (callbacks->fp(output) != 0)
		{
			MUGGLE_LOG_ERROR("DES crypt failed, failed FP");
			return -1;
		}

		// Feistel, 16 rounds
		for (int i = 15; i >= 0; --i)
		{
			tmp = output->u32.l;
			output->u32.l = output->u32.h;
			output->u32.h = tmp;

			t = callbacks->ep(output->u32.h);
			t.u64 ^= ks->subkey[i].u64;
			s = callbacks->sbox(t);
			output->u32.l ^= s;
		}

		// IP
		if (callbacks->ip(output) != 0)
		{
			MUGGLE_LOG_ERROR("DES crypt failed, failed IP");
			return -1;
		}
	}


	tmp = output->u32.l;
	output->u32.l = output->u32.h;
	output->u32.h = tmp;

	return 0;
}

int muggle_des_cipher(
	int mode,
	int enc,
	const unsigned char *input,
	unsigned char *output,
	unsigned int num_bytes,
	muggle_des_key_block key,
	muggle_64bit_block_t *iv,
	int update_iv,
	muggle_des_cb_t *callbacks)
{
	if (mode < 0 || mode >= MAX_MUGGLE_BLOCK_CIPHER_MODE)
	{
		MUGGLE_LOG_ERROR("invalid block cipher mode: %d", mode);
		return -1;
	}

	if (ROUND_UP_POW_OF_2_MUL(num_bytes, 8) != num_bytes)
	{
		MUGGLE_LOG_ERROR("DES cipher failed: input bytes is not multiple of 8");
		return -1;
	}
	size_t len = num_bytes / 8;

	// generate subkey
	int ret = 0;
	muggle_des_subkeys_t ks;
	if (callbacks == NULL)
	{
		ret = des_default_gen_subkey(&key, &ks);
	}
	else
	{
		ret = callbacks->gen_subkey(&key, &ks);
	}

	if (ret != 0)
	{
		MUGGLE_LOG_ERROR("DES cipher failed, failed gen subkey");
		return -1;
	}

	return s_fn_muggle_des[mode](enc, input, output, num_bytes, &ks, iv, update_iv, callbacks);
}

int muggle_des_ecb(
	int enc, const unsigned char *input, unsigned char *output, unsigned int num_bytes,
	muggle_des_subkeys_t *ks, muggle_64bit_block_t *iv,	int update_iv, muggle_des_cb_t *callbacks)
{
	int ret = 0;
	size_t len = num_bytes / 8, offset = 0;
	muggle_64bit_block_t *input_block, *output_block;
	for (size_t i = 0; i < len; ++i)
	{
		input_block = (muggle_64bit_block_t*)(input + offset);
		output_block = (muggle_64bit_block_t*)(output + offset);

		ret = muggle_des_crypt(enc, input_block, output_block, ks, callbacks);
		if (ret != 0)
		{
			MUGGLE_LOG_ERROR("DES ECB crypt failed");
			return ret;
		}
		offset += 8;
	}

	return 0;
}

int muggle_des_cbc(
	int enc, const unsigned char *input, unsigned char *output, unsigned int num_bytes,
	muggle_des_subkeys_t *ks, muggle_64bit_block_t *iv,	int update_iv, muggle_des_cb_t *callbacks)
{
	int ret = 0;
	size_t len = num_bytes / 8, offset = 0;
	muggle_64bit_block_t v;
	v.u64 = iv->u64;
	muggle_64bit_block_t *input_block, *output_block;
	for (size_t i = 0; i < len; ++i)
	{
		input_block = (muggle_64bit_block_t*)(input + offset);
		output_block = (muggle_64bit_block_t*)(output + offset);

		if (enc)
		{
			v.u64 ^= input_block->u64;
			ret = muggle_des_crypt(enc, &v, output_block, ks, callbacks);
			if (ret != 0)
			{
				MUGGLE_LOG_ERROR("DES ECB crypt failed");
				return ret;
			}

			v.u64 = output_block->u64;
		}
		else
		{
			ret = muggle_des_crypt(enc, input_block, output_block, ks, callbacks);
			if (ret != 0)
			{
				MUGGLE_LOG_ERROR("DES ECB crypt failed");
				return ret;
			}
			output_block->u64 ^= v.u64;

			v.u64 = input_block->u64;
		}

		offset += 8;
	}

	if (update_iv)
	{
		iv->u64 = v.u64;
	}

	return 0;
}

int muggle_des_cfb(
	int enc, const unsigned char *input, unsigned char *output, unsigned int num_bytes,
	muggle_des_subkeys_t *ks, muggle_64bit_block_t *iv,	int update_iv, muggle_des_cb_t *callbacks)
{
	int ret = 0;
	size_t len = num_bytes / 8, offset = 0;
	muggle_64bit_block_t v;
	v.u64 = iv->u64;
	muggle_64bit_block_t *input_block, *output_block;
	for (size_t i = 0; i < len; ++i)
	{
		input_block = (muggle_64bit_block_t*)(input + offset);
		output_block = (muggle_64bit_block_t*)(output + offset);

		if (enc)
		{
			ret = muggle_des_crypt(enc, &v, output_block, ks, callbacks);
			if (ret != 0)
			{
				MUGGLE_LOG_ERROR("DES CFB crypt failed");
				return ret;
			}
			output_block->u64 ^= input_block->u64;
			v.u64 = output_block->u64;
		}
		else
		{
			ret = muggle_des_crypt(!enc, &v, output_block, ks, callbacks);
			if (ret != 0)
			{
				MUGGLE_LOG_ERROR("DES CFB crypt failed");
				return ret;
			}
			output_block->u64 ^= input_block->u64;
			v.u64 = input_block->u64;
		}

		offset += 8;
	}

	if (update_iv)
	{
		iv->u64 = v.u64;
	}

	return 0;
}

int muggle_des_ofb(
	int enc, const unsigned char *input, unsigned char *output, unsigned int num_bytes,
	muggle_des_subkeys_t *ks, muggle_64bit_block_t *iv,	int update_iv, muggle_des_cb_t *callbacks)
{
	int ret = 0;
	size_t len = num_bytes / 8, offset = 0;
	muggle_64bit_block_t v;
	v.u64 = iv->u64;
	muggle_64bit_block_t *input_block, *output_block;
	for (size_t i = 0; i < len; ++i)
	{
		input_block = (muggle_64bit_block_t*)(input + offset);
		output_block = (muggle_64bit_block_t*)(output + offset);

		if (enc)
		{
			ret = muggle_des_crypt(enc, &v, output_block, ks, callbacks);
			if (ret != 0)
			{
				MUGGLE_LOG_ERROR("DES OFB crypt failed");
				return ret;
			}
			v.u64 = output_block->u64;
			output_block->u64 ^= input_block->u64;
		}
		else
		{
			ret = muggle_des_crypt(!enc, &v, output_block, ks, callbacks);
			if (ret != 0)
			{
				MUGGLE_LOG_ERROR("DES OFB crypt failed");
				return ret;
			}
			v.u64 = output_block->u64;
			output_block->u64 ^= input_block->u64;
		}

		offset += 8;
	}

	if (update_iv)
	{
		iv->u64 = v.u64;
	}

	return 0;
}

int muggle_des_ctr(
	int enc, const unsigned char *input, unsigned char *output, unsigned int num_bytes,
	muggle_des_subkeys_t *ks, muggle_64bit_block_t *iv,	int update_iv, muggle_des_cb_t *callbacks)
{
	int ret = 0;
	size_t len = num_bytes / 8, offset = 0;
	muggle_64bit_block_t v;
	v.u64 = iv->u64;
	muggle_64bit_block_t *input_block, *output_block;
	for (size_t i = 0; i < len; ++i)
	{
		input_block = (muggle_64bit_block_t*)(input + offset);
		output_block = (muggle_64bit_block_t*)(output + offset);

		if (enc)
		{
			ret = muggle_des_crypt(enc, &v, output_block, ks, callbacks);
			if (ret != 0)
			{
				MUGGLE_LOG_ERROR("DES CTR crypt failed");
				return ret;
			}
			output_block->u64 ^= input_block->u64;
			v.u64++;
		}
		else
		{
			ret = muggle_des_crypt(!enc, &v, output_block, ks, callbacks);
			if (ret != 0)
			{
				MUGGLE_LOG_ERROR("DES CTR crypt failed");
				return ret;
			}
			output_block->u64 ^= input_block->u64;
			v.u64++;
		}

		offset += 8;
	}

	if (update_iv)
	{
		iv->u64 = v.u64;
	}

	return 0;
}

