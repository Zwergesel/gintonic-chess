#include <cmath>

#include "chessboard.hpp"
#include "evaluator.hpp"
#include "magic.hpp"
#include "score.hpp"

using namespace ChessBoardConstants;

// Calculates a score for the current position from the moving player's point of view
score_t Evaluator::evaluatePosition(const ChessBoard& board)
{
	// Value is first calculated as positive for white and negative for black
	// At the end of the function the result is flipped if it is black's turn
	score_t value = 0;
	
	// Detect endgame
	u8 nQueens = Magic::count(board.mask_[white | queen] | board.mask_[black | queen]);
	u8 nRooks = Magic::count(board.mask_[white | rook] | board.mask_[black | rook]);
	u8 nBishops = Magic::count(board.mask_[white | bishop] | board.mask_[black | bishop]);
	u8 nKnights = Magic::count(board.mask_[white | knight] | board.mask_[black | knight]);
	
	float endgame = (5 * nQueens + 2 * nRooks + nBishops + nKnights); // [0, 26]
	endgame = 1.0f - std::min(1.0f, std::max(0.0f, (endgame - 10.0f) / 8.0f));
	
	// Evaluate pieces
	bitboard_t allPieces = board.occupied_;
	
	while (allPieces) {
		square_t square = Magic::extractBit(allPieces);
		piece_t piece = board.board_[square];
		value += Score::pieces[piece];
		
		switch(piece) {
		// KINGS
		case white | king:
			value += (score_t) ((1.0f - endgame) * Score::king_squares_midgame[square ^ mirror_square]);
			value += (score_t) (endgame * Score::king_squares_endgame[square ^ mirror_square]);
			break;
		case black | king:
			value -= (score_t) ((1.0f - endgame) * Score::king_squares_midgame[square]);
			value -= (score_t) (endgame * Score::king_squares_endgame[square]);
			break;
			
		// KNIGHTS
		case white | knight:
			value += Score::knight_squares[square ^ mirror_square];
			break;
		case black | knight:
			value -= Score::knight_squares[square];
			break;
			
		// BISHOPS
		case white | bishop:
			value += Score::bishop_squares[square ^ mirror_square];
			break;
		case black | bishop:
			value -= Score::bishop_squares[square];
			break;
			
		// ROOKS
		case white | rook:
			break;
		case black | rook:
			break;
			
		// QUEENS
		case white | queen:
			value += Score::queen_squares[square ^ mirror_square];
			break;
		case black | queen:
			value -= Score::queen_squares[square];
			break;
			
		// PAWNS
		case white | pawn:
			value += Score::pawn_squares[square ^ mirror_square];
			break;
		case black | pawn:
			value -= Score::pawn_squares[square];
			break;
		}
	}
	
	// Evaluate other strategic concepts
	
	if (board.player_ == black) value = -value;
	return (value / 10);
}