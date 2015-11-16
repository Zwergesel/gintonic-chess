#pragma once

#include "types.hpp"

namespace Magic
{
	const u64 firstBitMagicNumber = 0x022fdd63cc95386dL;
	const square_t firstBitLookup[] =
	{
		 0,  1,  2, 53,  3,  7, 54, 27,
		 4, 38, 41,  8, 34, 55, 48, 28,
		62,  5, 39, 46, 44, 42, 22,  9,
		24, 35, 59, 56, 49, 18, 29, 11,
		63, 52,  6, 26, 37, 40, 33, 47,
		61, 45, 43, 21, 23, 58, 17, 10,
		51, 25, 36, 32, 60, 20, 57, 16,
		50, 31, 19, 15, 30, 14, 13, 12,
	};
	
	inline square_t firstBit(bitboard_t bb)
	{
		return firstBitLookup[(int) (((bb&((~bb)+1))*firstBitMagicNumber) >> 58)];
	}
	
	inline square_t extractBit(bitboard_t& bitboard)
	{
		bitboard_t isolated_bit = bitboard & ((~bitboard) + 1);
		bitboard ^= isolated_bit;
		return firstBitLookup[(isolated_bit * firstBitMagicNumber) >> 58];
	}
	
	inline u8 count(bitboard_t bitboard)
	{
		u8 c = 0;
		for (; bitboard; bitboard &= bitboard - 1) ++c;
		return c;
	}
	
	inline bitboard_t mirrorBoard(bitboard_t bitboard)
	{
		bitboard_t result = 0L;
		for (u32 i=8; i; --i) {
			result |= bitboard & 0xff;
			bitboard >>= 8;
			result <<= 8;
		}
		return result;
	}
}