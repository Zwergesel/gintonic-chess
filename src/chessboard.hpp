#pragma once

#include <iostream>
#include <stack>
#include <vector>
#include "data.hpp"
#include "types.hpp"

struct HistoryInfo
{
	u8 capture;
	u8 castling;
	u8 enpassant;
	u8 drawmoves;
	u64 zobrist;
};

namespace ChessBoardConstants
{
	enum piece_e : char {
		nothing =	0,
		pawn =		1,
		knight =	2,
		king =		3,
		bishop =	5,
		rook =		6,
		queen =		7,
	};
	
	enum player_e {
		white = 0,
		black = 8,
		opponent = 8,
	};
	
	enum masks_e {
		mask_1bit = 0x1,
		mask_2bit = 0x3,
		mask_3bit = 0x7,
		mask_4bit = 0xf,
		mask_5bit = 0x1f,
		mask_6bit = 0x3f,
		mask_7bit = 0x7f,
		mask_8bit = 0xff,
		mask_color = 0x8,
		mask_piecetype = 0x7,
	};
	
	// xor with square_t to mirror along horizontal
	const square_t mirror_square = 56;
}

class ChessBoard
{
public:
	// Set position
	bool setPosition(const tokenizer& tokens, tokenizer::iterator& token);
	void setInitialPosition();
	
	// Generate moves
	void generateMoves(std::vector<move_t>& movelist);
	void generateGoodCaptures(std::vector<move_t>& movelist);
	void generateAttacks(std::vector<move_t>& movelist) const;
	void sortMoves(std::vector<move_t>& movelist, move_t sortFirst) const;
	bool isKingAttacked(player_t color) const;
	
	// Perform moves
	void doMove(move_t move);
	void undoMove(move_t move);
	bool isValidMove(move_t move) const;
	bool lastMoveWasQuiet() const;
	
	// Debug printing
	void printBoard(std::ostream& out) const;
	static void printBitboard(std::ostream& out, bitboard_t bitboard);
	void printMove(std::ostream& out, move_t move) const;
	void printDebug(std::ostream& out) const;
	
	// Parsing
	static std::string nameSquare(square_t square);
	move_t parseMove(std::string move) const;
	std::string uciMove(move_t move) const;
	
	// Position data
	bitboard_t occupied_;
	bitboard_t mask_[16];
	piece_t board_[64];
	
	u64 zobrist_;
	
	player_t player_;	// Current player
	u8 castling_;		// 4 Flags for the allowed castles
	square_t enpassant_;// Position at which a pawn may be captured en passant next move (e.g. f3)
	u8 drawmoves_;		// Counter for 50-move draw
	u16 movenumber_;	// Current move number (starts at 1, increases after black's move)
	
	std::stack<HistoryInfo> history_;
	
private:
	// Generate moves
	void generateMovesKing(std::vector<move_t>& movelist, bitboard_t allowed) const;
	void generateMovesKnight(std::vector<move_t>& movelist, bitboard_t allowed) const;
	void generateMovesSliding(std::vector<move_t>& movelist, piece_t type, bitboard_t allowed) const;
	void generateMovesPawn(std::vector<move_t>& movelist, bool captures_only) const;
	void generateCastles(std::vector<move_t>& movelist) const;
	int isSquareAttacked(square_t square, player_t color) const;
	bool leavesKingInCheck(move_t move);
	
	// Set position
	void rebuildZobrist();
	bool parseFEN(const std::string& fen);
	bool parsePlayer(const std::string& player);
	bool parseCastling(const std::string& castling);
	bool parseEnpassant(const std::string& enpassantSquare);
	static square_t parseSquare(const std::string& square);
	
	const std::string namePiece[8] = {
		"nothing", "pawn", "knight", "king", "error", "bishop", "rook", "queen"
	};
	
	inline void remove_piece(square_t square)
	{
		using namespace ChessBoardConstants;
		bitboard_t clear_bit = ~BIT(square);
		occupied_ &= clear_bit;
		mask_[board_[square] & mask_color] &= clear_bit;
		mask_[board_[square]] &= clear_bit;
		switch_zobrist(square | (board_[square] << 6));
		board_[square] = nothing;
	}
	
	inline void insert_piece(square_t square, piece_t piece)
	{
		using namespace ChessBoardConstants;
		bitboard_t set_bit = BIT(square);
		occupied_ |= set_bit;
		mask_[piece & mask_color] |= set_bit;
		mask_[piece] |= set_bit;
		switch_zobrist(square | (piece << 6));
		board_[square] = piece;
	}
	
	inline void switch_zobrist(u16 zobrist_key)
	{
		zobrist_ ^= Data::zobrist[zobrist_key];
	}
	
};
