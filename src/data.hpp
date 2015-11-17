#pragma once

#include "types.hpp"

namespace Data
{
	
	// Special move types
	const u8 move_quiet = 0;
	const u8 move_promotion_knight = 1;
	const u8 move_promotion_bishop = 2;
	const u8 move_promotion_rook = 3;
	const u8 move_promotion_queen = 4;
	const u8 move_castle_kingside = 5;
	const u8 move_castle_queenside = 6;
	const u8 move_double_pawn_push = 7;
	const u8 move_enpassant_capture = 8;
	
	// Squares
	const square_t square_a1 = 0;
	const square_t square_c1 = 2;
	const square_t square_d1 = 3;
	const square_t square_e1 = 4;
	const square_t square_f1 = 5;
	const square_t square_g1 = 6;
	const square_t square_h1 = 7;
	const square_t square_a8 = 56;
	const square_t square_c8 = 58;
	const square_t square_d8 = 59;
	const square_t square_e8 = 60;
	const square_t square_f8 = 61;
	const square_t square_g8 = 62;
	const square_t square_h8 = 63;
	
	// Castling flags
	const u8 castle_white_kingside =	0x1;
	const u8 castle_white_queenside =	0x2;
	const u8 castle_white =				0x3;
	const u8 castle_black_kingside =	0x4;
	const u8 castle_black_queenside =	0x8;
	const u8 castle_black =				0xc;
	
	// Castling squares that may not be occupied
	const bitboard_t castle_squares_white_kingside = 0x60L;
	const bitboard_t castle_squares_white_queenside = 0xeL;
	const bitboard_t castle_squares_black_kingside = 0x6000000000000000L;
	const bitboard_t castle_squares_black_queenside = 0xe00000000000000L;
	
	// Castling moves
	const move_t fullmove_castle_white_k = square_e1 | (square_g1 << 6) | (move_castle_kingside << 12);
	const move_t fullmove_castle_white_q = square_e1 | (square_c1 << 6) | (move_castle_queenside << 12);
	const move_t fullmove_castle_black_k = square_e8 | (square_g8 << 6) | (move_castle_kingside << 12);
	const move_t fullmove_castle_black_q = square_e8 | (square_c8 << 6) | (move_castle_queenside << 12);
	
	// Lines and files
	const bitboard_t line[] = {
		0xffL, 0xff00L, 0xff0000L, 0xff000000L, 0xff00000000L, 0xff0000000000L,
		0xff000000000000L, 0xff00000000000000L
	};
	const bitboard_t file[] = {
		0x0101010101010101L, 0x0202020202020202L, 0x0404040404040404L, 0x0808080808080808L,
		0x1010101010101010L, 0x2020202020202020L, 0x4040404040404040L, 0x8080808080808080L
	};
	
	const bitboard_t all_squares = 0xffffffffffffffffL;
	
	// Castling bits to be kept for a move from/to a specific square
	const u8 update_castling[64] = {
		13, 15, 15, 15, 12, 15, 15, 14,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		7, 15, 15, 15,  3, 15, 15, 11,
	};
	
	// Priority for move ordering
	// Entry p[a][b] means piecetype 'a' moves and captures piecetype 'b'
	// b can be 'nothing'
	const int priorityCaptures[8][8] = {
		{},
		{ 40, 31, 47, 0, 0, 48, 54, 58 }, // pawn
		{ 43, 24, 36, 0, 0, 37, 52, 57 }, // knight
		{ 30, 46, 49, 0, 0, 50, 55, 59 }, // king
		{},
		{ 42, 23, 35, 0, 0, 34, 51, 56 }, // bishop
		{ 38, 22, 27, 0, 0, 28, 32, 53 }, // rook
		{ 39, 21, 25, 0, 0, 26, 29, 33 }, // queen
	};
	const int prioritySpecial[16] = { 0, -39, -40, -38, 20, 15, 14, 1, 0, 0, 0, 0, 0, 0, 0, 0 };
	const int priorityMax = 99;
	
	extern bitboard_t attacks_king[];
	extern bitboard_t attacks_knight[];
	
	// Zobrist format:
	// 6 bit = square
	// 3 bit = piecetype
	// 1 bit = player_color
	// Zobrist special values:
	// black to move	=> 0100111111
	// castling			=> 010000CCCC
	// enpassant		=> 1100EEEEEE
	
	const u64 zobrist_player = 0x13f;
	const u64 zobrist_castling = 0x100;
	const u64 zobrist_enpassant = 0x300;
	extern u64 zobrist[];
	
	void initialize();
	
};