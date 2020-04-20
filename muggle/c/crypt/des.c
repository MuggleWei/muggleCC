/*
 *	author: muggle wei <mugglewei@gmail.com>
 *
 *	Use of this source code is governed by the MIT license that can be
 *	found in the LICENSE file.
 */

#include "des.h"
#include <stddef.h>
#include <string.h>
#include "muggle/c/base/utils.h"
#include "muggle/c/log/log.h"

/*
 * move single bit
 * */
// #define MOVE_SINGLE_BIT(in, from, to) (((uint32_t)((in_bytes[(from)/8])>>(from%8))&0x01)<<to)
#define MOVE_SINGLE_BIT(in, from, to) ((((in)>>from)&0x01)<<to)

/*
 * key shift
 * */
#define DES_KEY_SHIFT(in, shift, mask) (((in)<<(shift))|(((in)>>(28-(shift)))&(mask)))

static uint32_t s_muggle_des_sbox_table[8][64] = {
	{
		// S1
		14, 4,  13, 1, 2,  15, 11, 8,  3,  10, 6,  12, 5,  9,  0, 7,
		0,  15, 7,  4, 14, 2,  13, 1,  10, 6,  12, 11, 9,  5,  3, 8,
		4,  1,  14, 8, 13, 6,  2,  11, 15, 12, 9,  7,  3,  10, 5, 0,
		15, 12, 8,  2, 4,  9,  1,  7,  5,  11, 3,  14, 10, 0,  6, 13
	},
	{
		// S2
		15, 1,  8,  14, 6,  11, 3,  4,  9,  7, 2,  13, 12, 0, 5,  10,
		3,  13, 4,  7,  15, 2,  8,  14, 12, 0, 1,  10, 6,  9, 11, 5,
		0,  14, 7,  11, 10, 4,  13, 1,  5,  8, 12, 6,  9,  3, 2,  15,
		13, 8,  10, 1,  3,  15, 4,  2,  11, 6, 7,  12, 0,  5, 14, 9
	},
	{
		// S3
		10, 0,  9,  14, 6, 3,  15, 5,  1,  13, 12, 7,  11, 4,  2,  8,
		13, 7,  0,  9,  3, 4,  6,  10, 2,  8,  5,  14, 12, 11, 15, 1,
		13, 6,  4,  9,  8, 15, 3,  0,  11, 1,  2,  12, 5,  10, 14, 7,
		1,  10, 13, 0,  6, 9,  8,  7,  4,  15, 14, 3,  11, 5,  2,  12
	},
	{
		// S4
		7,  13, 14, 3, 0,  6,  9,  10, 1,  2, 8, 5,  11, 12, 4,  15,
		13, 8,  11, 5, 6,  15, 0,  3,  4,  7, 2, 12, 1,  10, 14, 9,
		10, 6,  9,  0, 12, 11, 7,  13, 15, 1, 3, 14, 5,  2,  8,  4,
		3,  15, 0,  6, 10, 1,  13, 8,  9,  4, 5, 11, 12, 7,  2,  14
	},
	{
		// S5
		2,  12, 4,  1,  7,  10, 11, 6,  8,  5,  3,  15, 13, 0, 14, 9,
		14, 11, 2,  12, 4,  7,  13, 1,  5,  0,  15, 10, 3,  9, 8,  6,
		4,  2,  1,  11, 10, 13, 7,  8,  15, 9,  12, 5,  6,  3, 0,  14,
		11, 8,  12, 7,  1,  14, 2,  13, 6,  15, 0,  9,  10, 4, 5,  3
	},
	{
		// S6
		12, 1,  10, 15, 9, 2,  6,  8,  0,  13, 3,  4,  14, 7,  5,  11,
		10, 15, 4,  2,  7, 12, 9,  5,  6,  1,  13, 14, 0,  11, 3,  8,
		9,  14, 15, 5,  2, 8,  12, 3,  7,  0,  4,  10, 1,  13, 11, 6,
		4,  3,  2,  12, 9, 5,  15, 10, 11, 14, 1,  7,  6,  0,  8,  13
	},
	{
		// S7
		4,  11, 2,  14, 15, 0, 8,  13, 3,  12, 9, 7,  5,  10, 6, 1,
		13, 0,  11, 7,  4,  9, 1,  10, 14, 3,  5, 12, 2,  15, 8, 6,
		1,  4,  11, 13, 12, 3, 7,  14, 10, 15, 6, 8,  0,  5,  9, 2,
		6,  11, 13, 8,  1,  4, 10, 7,  9,  5,  0, 15, 14, 2,  3, 12
	},
	{
		// S8
		13, 2,  8,  4, 6,  15, 11, 1,  10, 9,  3,  14, 5,  0,  12, 7,
		1,  15, 13, 8, 10, 3,  7,  4,  12, 5,  6,  11, 0,  14, 9,  2,
		7,  11, 4,  1, 9,  12, 14, 2,  0,  6,  10, 13, 15, 3,  5,  8,
		2,  1,  14, 7, 4,  10, 8,  13, 15, 12, 9,  0,  3,  5,  6,  11
	}
};

void muggle_des_ip(const muggle_64bit_block_t *in, muggle_64bit_block_t *out)
{
	/*
	 * Initial Permutation table
	 *
	 *   58  50  42  34  26  18  10   2
	 *   60  52  44  36  28  20  12   4
	 *   62  54  46  38  30  22  14   6
	 *   64  56  48  40  32  24  16   8
	 *   57  49  41  33  25  17   9   1
	 *   59  51  43  35  27  19  11   3
	 *   61  53  45  37  29  21  13   5
	 *   63  55  47  39  31  23  15   7
	 *
	 * */
	memset(out, 0, sizeof(muggle_64bit_block_t));

	// move bit one by one
	out->u32.l = (uint32_t)(
		MOVE_SINGLE_BIT(in->u64, 57, 0)  | MOVE_SINGLE_BIT(in->u64, 49, 1)  | MOVE_SINGLE_BIT(in->u64, 41, 2)  | MOVE_SINGLE_BIT(in->u64, 33, 3)  |
		MOVE_SINGLE_BIT(in->u64, 25, 4)  | MOVE_SINGLE_BIT(in->u64, 17, 5)  | MOVE_SINGLE_BIT(in->u64, 9, 6)   | MOVE_SINGLE_BIT(in->u64, 1, 7)   |
		MOVE_SINGLE_BIT(in->u64, 59, 8)  | MOVE_SINGLE_BIT(in->u64, 51, 9)  | MOVE_SINGLE_BIT(in->u64, 43, 10) | MOVE_SINGLE_BIT(in->u64, 35, 11) |
		MOVE_SINGLE_BIT(in->u64, 27, 12) | MOVE_SINGLE_BIT(in->u64, 19, 13) | MOVE_SINGLE_BIT(in->u64, 11, 14) | MOVE_SINGLE_BIT(in->u64, 3, 15)  |
		MOVE_SINGLE_BIT(in->u64, 61, 16) | MOVE_SINGLE_BIT(in->u64, 53, 17) | MOVE_SINGLE_BIT(in->u64, 45, 18) | MOVE_SINGLE_BIT(in->u64, 37, 19) |
		MOVE_SINGLE_BIT(in->u64, 29, 20) | MOVE_SINGLE_BIT(in->u64, 21, 21) | MOVE_SINGLE_BIT(in->u64, 13, 22) | MOVE_SINGLE_BIT(in->u64, 5, 23)  |
		MOVE_SINGLE_BIT(in->u64, 63, 24) | MOVE_SINGLE_BIT(in->u64, 55, 25) | MOVE_SINGLE_BIT(in->u64, 47, 26) | MOVE_SINGLE_BIT(in->u64, 39, 27) |
		MOVE_SINGLE_BIT(in->u64, 31, 28) | MOVE_SINGLE_BIT(in->u64, 23, 29) | MOVE_SINGLE_BIT(in->u64, 15, 30) | MOVE_SINGLE_BIT(in->u64, 7, 31)
	);
	out->u32.h = (uint32_t)(
		MOVE_SINGLE_BIT(in->u64, 56, 0)  | MOVE_SINGLE_BIT(in->u64, 48, 1)  | MOVE_SINGLE_BIT(in->u64, 40, 2)  | MOVE_SINGLE_BIT(in->u64, 32, 3)  |
		MOVE_SINGLE_BIT(in->u64, 24, 4)  | MOVE_SINGLE_BIT(in->u64, 16, 5)  | MOVE_SINGLE_BIT(in->u64, 8, 6)   | MOVE_SINGLE_BIT(in->u64, 0, 7)   |
		MOVE_SINGLE_BIT(in->u64, 58, 8)  | MOVE_SINGLE_BIT(in->u64, 50, 9)  | MOVE_SINGLE_BIT(in->u64, 42, 10) | MOVE_SINGLE_BIT(in->u64, 34, 11) |
		MOVE_SINGLE_BIT(in->u64, 26, 12) | MOVE_SINGLE_BIT(in->u64, 18, 13) | MOVE_SINGLE_BIT(in->u64, 10, 14) | MOVE_SINGLE_BIT(in->u64, 2, 15)  |
		MOVE_SINGLE_BIT(in->u64, 60, 16) | MOVE_SINGLE_BIT(in->u64, 52, 17) | MOVE_SINGLE_BIT(in->u64, 44, 18) | MOVE_SINGLE_BIT(in->u64, 36, 19) |
		MOVE_SINGLE_BIT(in->u64, 28, 20) | MOVE_SINGLE_BIT(in->u64, 20, 21) | MOVE_SINGLE_BIT(in->u64, 12, 22) | MOVE_SINGLE_BIT(in->u64, 4, 23)  |
		MOVE_SINGLE_BIT(in->u64, 62, 24) | MOVE_SINGLE_BIT(in->u64, 54, 25) | MOVE_SINGLE_BIT(in->u64, 46, 26) | MOVE_SINGLE_BIT(in->u64, 38, 27) |
		MOVE_SINGLE_BIT(in->u64, 30, 28) | MOVE_SINGLE_BIT(in->u64, 22, 29) | MOVE_SINGLE_BIT(in->u64, 14, 30) | MOVE_SINGLE_BIT(in->u64, 6, 31)
	);
}

void muggle_des_fp(const muggle_64bit_block_t *in, muggle_64bit_block_t *out)
{
	/*
	 * Final Permutation(Inv Inital Permutation) table
	 *
	 *   40   8  48  16  56  24  64  32
	 *   39   7  47  15  55  23  63  31
	 *   38   6  46  14  54  22  62  30
	 *   37   5  45  13  53  21  61  29
	 *   36   4  44  12  52  20  60  28
	 *   35   3  43  11  51  19  59  27
	 *   34   2  42  10  50  18  58  26
	 *   33   1  41   9  49  17  57  25
	 *
	 * */
	memset(out, 0, sizeof(muggle_64bit_block_t));

	// move bit one by one
	out->u32.l = (uint32_t)(
		MOVE_SINGLE_BIT(in->u64, 39, 0)  | MOVE_SINGLE_BIT(in->u64, 7, 1)   | MOVE_SINGLE_BIT(in->u64, 47, 2)  | MOVE_SINGLE_BIT(in->u64, 15, 3)  |
		MOVE_SINGLE_BIT(in->u64, 55, 4)  | MOVE_SINGLE_BIT(in->u64, 23, 5)  | MOVE_SINGLE_BIT(in->u64, 63, 6)  | MOVE_SINGLE_BIT(in->u64, 31, 7)  |
		MOVE_SINGLE_BIT(in->u64, 38, 8)  | MOVE_SINGLE_BIT(in->u64, 6, 9)   | MOVE_SINGLE_BIT(in->u64, 46, 10) | MOVE_SINGLE_BIT(in->u64, 14, 11) |
		MOVE_SINGLE_BIT(in->u64, 54, 12) | MOVE_SINGLE_BIT(in->u64, 22, 13) | MOVE_SINGLE_BIT(in->u64, 62, 14) | MOVE_SINGLE_BIT(in->u64, 30, 15) |
		MOVE_SINGLE_BIT(in->u64, 37, 16) | MOVE_SINGLE_BIT(in->u64, 5, 17)  | MOVE_SINGLE_BIT(in->u64, 45, 18) | MOVE_SINGLE_BIT(in->u64, 13, 19) |
		MOVE_SINGLE_BIT(in->u64, 53, 20) | MOVE_SINGLE_BIT(in->u64, 21, 21) | MOVE_SINGLE_BIT(in->u64, 61, 22) | MOVE_SINGLE_BIT(in->u64, 29, 23) |
		MOVE_SINGLE_BIT(in->u64, 36, 24) | MOVE_SINGLE_BIT(in->u64, 4, 25)  | MOVE_SINGLE_BIT(in->u64, 44, 26) | MOVE_SINGLE_BIT(in->u64, 12, 27) |
		MOVE_SINGLE_BIT(in->u64, 52, 28) | MOVE_SINGLE_BIT(in->u64, 20, 29) | MOVE_SINGLE_BIT(in->u64, 60, 30) | MOVE_SINGLE_BIT(in->u64, 28, 31)
	);
	out->u32.h = (uint32_t)(
		MOVE_SINGLE_BIT(in->u64, 35, 0)  | MOVE_SINGLE_BIT(in->u64, 3, 1)   | MOVE_SINGLE_BIT(in->u64, 43, 2)  | MOVE_SINGLE_BIT(in->u64, 11, 3)  |
		MOVE_SINGLE_BIT(in->u64, 51, 4)  | MOVE_SINGLE_BIT(in->u64, 19, 5)  | MOVE_SINGLE_BIT(in->u64, 59, 6)  | MOVE_SINGLE_BIT(in->u64, 27, 7)  |
		MOVE_SINGLE_BIT(in->u64, 34, 8)  | MOVE_SINGLE_BIT(in->u64, 2, 9)   | MOVE_SINGLE_BIT(in->u64, 42, 10) | MOVE_SINGLE_BIT(in->u64, 10, 11) |
		MOVE_SINGLE_BIT(in->u64, 50, 12) | MOVE_SINGLE_BIT(in->u64, 18, 13) | MOVE_SINGLE_BIT(in->u64, 58, 14) | MOVE_SINGLE_BIT(in->u64, 26, 15) |
		MOVE_SINGLE_BIT(in->u64, 33, 16) | MOVE_SINGLE_BIT(in->u64, 1, 17)  | MOVE_SINGLE_BIT(in->u64, 41, 18) | MOVE_SINGLE_BIT(in->u64, 9, 19)  |
		MOVE_SINGLE_BIT(in->u64, 49, 20) | MOVE_SINGLE_BIT(in->u64, 17, 21) | MOVE_SINGLE_BIT(in->u64, 57, 22) | MOVE_SINGLE_BIT(in->u64, 25, 23) |
		MOVE_SINGLE_BIT(in->u64, 32, 24) | MOVE_SINGLE_BIT(in->u64, 0, 25)  | MOVE_SINGLE_BIT(in->u64, 40, 26) | MOVE_SINGLE_BIT(in->u64, 8, 27)  |
		MOVE_SINGLE_BIT(in->u64, 48, 28) | MOVE_SINGLE_BIT(in->u64, 16, 29) | MOVE_SINGLE_BIT(in->u64, 56, 30) | MOVE_SINGLE_BIT(in->u64, 24, 31)
	);
}

void muggle_des_expand(const muggle_32bit_block_t *in, muggle_des_subkey_t *out)
{
	/*
	 * Expand Permutation
	 *
	 *   32   1   2   3   4   5 
	 *    4   5   6   7   8   9
	 *    8   9  10  11  12  13
	 *   12  13  14  15  16  17
	 *   16  17  18  19  20  21
	 *   20  21  22  23  24  25
	 *   24  25  26  27  28  29
	 *   28  29  30  31  32   1
	 * */

	uint32_t u32 = in->u32;
	out->bytes[0] = (unsigned char)(((u32 >> 31) & 0x01) | (u32 << 1));
	out->bytes[1] = (unsigned char)(u32 >> 3);
	out->bytes[2] = (unsigned char)(u32 >> 7);
	out->bytes[3] = (unsigned char)(u32 >> 11);
	out->bytes[4] = (unsigned char)(u32 >> 15);
	out->bytes[5] = (unsigned char)(u32 >> 19);
	out->bytes[6] = (unsigned char)(u32 >> 23);
	out->bytes[7] = (unsigned char)(((u32 >> 27) & 0x1f) | (u32 << 31));
}

void muggle_des_sbox(const muggle_des_subkey_t *in, muggle_des_subkey_t *out)
{
	for (int i = 0; i < 8; i++)
	{
		unsigned char b = in->bytes[i];
		unsigned char row = (b & 0x01) + ((b >> 4) & 0x02);
		unsigned char col = (b >> 1) & 0x0f;
		out->bytes[i] = s_muggle_des_sbox_table[i][8 * row + col];
	}
}

void muggle_des_p(const muggle_32bit_block_t *in, muggle_32bit_block_t *out)
{
	/*
	 * P Permutation
	 *   16   7  20  21  29  12  28  17
	 *    1  15  23  26   5  18  31  10
	 *    2   8  24  14  32  27   3   9
	 *   19  13  30   6  22  11   4  25
	 * */
	out->u32 = (uint32_t)(
		MOVE_SINGLE_BIT(in->u32, 15, 0)   | MOVE_SINGLE_BIT(in->u32, 6,  1)  | MOVE_SINGLE_BIT(in->u32, 19, 2)  | MOVE_SINGLE_BIT(in->u32, 20, 3)   |
		MOVE_SINGLE_BIT(in->u32, 28, 4)   | MOVE_SINGLE_BIT(in->u32, 11, 5)  | MOVE_SINGLE_BIT(in->u32, 27, 6)  | MOVE_SINGLE_BIT(in->u32, 16, 7)   |
		MOVE_SINGLE_BIT(in->u32, 0,  8)   | MOVE_SINGLE_BIT(in->u32, 14, 9)  | MOVE_SINGLE_BIT(in->u32, 22, 10) | MOVE_SINGLE_BIT(in->u32, 25, 11)  |
		MOVE_SINGLE_BIT(in->u32, 4,  12)  | MOVE_SINGLE_BIT(in->u32, 17, 13) | MOVE_SINGLE_BIT(in->u32, 30, 14) | MOVE_SINGLE_BIT(in->u32, 9,  15)  |
		MOVE_SINGLE_BIT(in->u32, 1,  16)  | MOVE_SINGLE_BIT(in->u32, 7, 17)  | MOVE_SINGLE_BIT(in->u32, 23, 18) | MOVE_SINGLE_BIT(in->u32, 13,  19) |
		MOVE_SINGLE_BIT(in->u32, 31,  20) | MOVE_SINGLE_BIT(in->u32, 26, 21) | MOVE_SINGLE_BIT(in->u32, 2, 22)  | MOVE_SINGLE_BIT(in->u32, 8,  23)  |
		MOVE_SINGLE_BIT(in->u32, 18,  24) | MOVE_SINGLE_BIT(in->u32, 12, 25) | MOVE_SINGLE_BIT(in->u32, 29, 26) | MOVE_SINGLE_BIT(in->u32, 5,  27)  |
		MOVE_SINGLE_BIT(in->u32, 21,  28) | MOVE_SINGLE_BIT(in->u32, 10, 29) | MOVE_SINGLE_BIT(in->u32, 3, 30)  | MOVE_SINGLE_BIT(in->u32, 24,  31)
	);
}

void muggle_des_f(const muggle_32bit_block_t *in, const muggle_des_subkey_t *sk, muggle_32bit_block_t *out)
{
	muggle_des_subkey_t p;

	// Expand Permutation
	muggle_des_expand(in, &p);

	// XOR subkey
	for (int i = 0; i < 8; i++)
	{
		p.bytes[i] = p.bytes[i] ^ sk->bytes[i];
	}

	// S-Box Permutation
	muggle_des_sbox(&p, &p);

	const unsigned char *in_bytes = in->bytes;
	muggle_32bit_block_t u32_block;
	u32_block.u32 =
		((uint32_t)in_bytes[0]         & 0x0000000f) |
		(((uint32_t)in_bytes[1] << 4)  & 0x000000f0) |
		(((uint32_t)in_bytes[2] << 8)  & 0x00000f00) |
		(((uint32_t)in_bytes[3] << 12) & 0x0000f000) |
		(((uint32_t)in_bytes[4] << 16) & 0x000f0000) |
		(((uint32_t)in_bytes[5] << 20) & 0x00f00000) |
		(((uint32_t)in_bytes[6] << 24) & 0x0f000000) |
		(((uint32_t)in_bytes[7] << 28) & 0xf0000000);

	// P permutation
	muggle_des_p(&u32_block, out);
}

void muggle_des_pc1(const muggle_64bit_block_t *in, muggle_64bit_block_t *out)
{
	/*
	 * PC-1
	 *
	 * 57 49 41 33 25 17 9
	 * 1 58 50 42 34 26 18
	 * 10 2 59 51 43 35 27
	 * 19 11 3 60 52 44 36
	 * 
	 * 63 55 47 39 31 23 15
	 * 7 62 54 46 38 30 22
	 * 14 6 61 53 45 37 29
	 * 21 13 5 28 20 12 4
	 *
	 * */
	out->u32.l = (uint32_t)(
		MOVE_SINGLE_BIT(in->u64, 56, 0)  | MOVE_SINGLE_BIT(in->u64, 48, 1)  | MOVE_SINGLE_BIT(in->u64, 40, 2)  | MOVE_SINGLE_BIT(in->u64, 32, 3)  |
		MOVE_SINGLE_BIT(in->u64, 24, 4)  | MOVE_SINGLE_BIT(in->u64, 16, 5)  | MOVE_SINGLE_BIT(in->u64, 8, 6)   | MOVE_SINGLE_BIT(in->u64, 0, 7)   |
		MOVE_SINGLE_BIT(in->u64, 57, 8)  | MOVE_SINGLE_BIT(in->u64, 49, 9)  | MOVE_SINGLE_BIT(in->u64, 41, 10) | MOVE_SINGLE_BIT(in->u64, 33, 11) |
		MOVE_SINGLE_BIT(in->u64, 25, 12) | MOVE_SINGLE_BIT(in->u64, 17, 13) | MOVE_SINGLE_BIT(in->u64, 9, 14)  | MOVE_SINGLE_BIT(in->u64, 1, 15)  |
		MOVE_SINGLE_BIT(in->u64, 58, 16) | MOVE_SINGLE_BIT(in->u64, 50, 17) | MOVE_SINGLE_BIT(in->u64, 42, 18) | MOVE_SINGLE_BIT(in->u64, 34, 19) |
		MOVE_SINGLE_BIT(in->u64, 26, 20) | MOVE_SINGLE_BIT(in->u64, 18, 21) | MOVE_SINGLE_BIT(in->u64, 10, 22) | MOVE_SINGLE_BIT(in->u64, 2, 23)  |
		MOVE_SINGLE_BIT(in->u64, 59, 24) | MOVE_SINGLE_BIT(in->u64, 51, 25) | MOVE_SINGLE_BIT(in->u64, 43, 26) | MOVE_SINGLE_BIT(in->u64, 35, 27)
	);

	out->u32.h = (uint32_t)(
		MOVE_SINGLE_BIT(in->u64, 62, 0)  | MOVE_SINGLE_BIT(in->u64, 54, 1)  | MOVE_SINGLE_BIT(in->u64, 46, 2)  | MOVE_SINGLE_BIT(in->u64, 38, 3)  |
		MOVE_SINGLE_BIT(in->u64, 30, 4)  | MOVE_SINGLE_BIT(in->u64, 22, 5)  | MOVE_SINGLE_BIT(in->u64, 14, 6)  | MOVE_SINGLE_BIT(in->u64, 6, 7)   |
		MOVE_SINGLE_BIT(in->u64, 61, 8)  | MOVE_SINGLE_BIT(in->u64, 53, 9)  | MOVE_SINGLE_BIT(in->u64, 45, 10) | MOVE_SINGLE_BIT(in->u64, 37, 11) |
		MOVE_SINGLE_BIT(in->u64, 29, 12) | MOVE_SINGLE_BIT(in->u64, 21, 13) | MOVE_SINGLE_BIT(in->u64, 13, 14) | MOVE_SINGLE_BIT(in->u64, 5, 15)  |
		MOVE_SINGLE_BIT(in->u64, 60, 16) | MOVE_SINGLE_BIT(in->u64, 52, 17) | MOVE_SINGLE_BIT(in->u64, 44, 18) | MOVE_SINGLE_BIT(in->u64, 36, 19) |
		MOVE_SINGLE_BIT(in->u64, 28, 20) | MOVE_SINGLE_BIT(in->u64, 20, 21) | MOVE_SINGLE_BIT(in->u64, 12, 22) | MOVE_SINGLE_BIT(in->u64, 4, 23)  |
		MOVE_SINGLE_BIT(in->u64, 27, 24) | MOVE_SINGLE_BIT(in->u64, 19, 25) | MOVE_SINGLE_BIT(in->u64, 11, 26) | MOVE_SINGLE_BIT(in->u64, 3, 27)
	);
}

void muggle_des_gen_subkeys(const muggle_64bit_block_t *key, muggle_des_subkeys_t *subkeys)
{
	muggle_64bit_block_t out;

	// PC-1
	muggle_des_pc1(key, &out);

	uint32_t c = out.u32.l;
	uint32_t d = out.u32.h;

	c=DES_KEY_SHIFT(c, 1, 0x01);
	d=DES_KEY_SHIFT(d, 1, 0x01);

	// TODO:
}

/*
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
*/

