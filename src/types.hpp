#pragma once

#include <cstdint>
#include <boost/tokenizer.hpp>

typedef boost::tokenizer<boost::char_separator<char>> tokenizer;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef u16 player_t;
typedef u16 piece_t;
typedef u32 move_t;
typedef u64 bitboard_t;
typedef u16 square_t;

typedef int32_t score_t;

#define BIT(x) (1L << (x))