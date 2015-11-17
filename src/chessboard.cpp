#include <algorithm>
#include <cassert>
#include <sstream>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/regex.hpp>

#include "chessboard.hpp"
#include "magic.hpp"
#include "stdx.hpp"
#include "Crafty/MagicMoves.hpp"

using std::string;
using namespace ChessBoardConstants;

#define MOVE_FROM(move) ((move) & mask_6bit)
#define MOVE_TO(move) (((move) >> 6) & mask_6bit)
#define MOVE_SPECIAL(move) (((move) >> 12) & mask_4bit)
#define MAKE_MOVE_FT(from, to) ((from) | ((to) << 6))
#define MAKE_MOVE_FTS(from, to, special) ((from) | ((to) << 6) | ((special) << 12))

void ChessBoard::generateMoves(std::vector<move_t>& movelist)
{
	generateMovesKing(movelist, Data::all_squares);
	generateMovesKnight(movelist, Data::all_squares);
	generateMovesBishopRookQueen(movelist, Data::all_squares);
	generateMovesPawn(movelist, false);
	generateCastles(movelist);
	stdx::erase_if(movelist, [this](move_t m) { return leavesKingInCheck(m); });
}

void ChessBoard::generateGoodCaptures(std::vector<move_t>& movelist) const
{
}

void ChessBoard::generateAttacks(std::vector<move_t>& movelist) const
{
}

void ChessBoard::generateMovesKing(std::vector<move_t>& movelist, bitboard_t allowed) const
{
	if (mask_[player_ | king] == 0) return;
	square_t from = Magic::firstBit(mask_[player_ | king]);
	bitboard_t moves = Data::attacks_king[from];
	moves &= ~mask_[player_];
	moves &= allowed;
	while (moves) {
		square_t to = Magic::extractBit(moves);
		movelist.push_back(MAKE_MOVE_FT(from, to));
	}
}

void ChessBoard::generateMovesKnight(std::vector<move_t>& movelist, bitboard_t allowed) const
{
	bitboard_t myKnights = mask_[player_ | knight];
	while (myKnights != 0) {
		square_t from = Magic::extractBit(myKnights);
		bitboard_t moves = Data::attacks_knight[from];
		moves &= ~mask_[player_];
		moves &= allowed;
		while (moves) {
			int to = Magic::extractBit(moves);
			movelist.push_back(MAKE_MOVE_FT(from, to));
		}
	}
}

void ChessBoard::generateMovesBishopRookQueen(std::vector<move_t>& movelist, bitboard_t allowed) const
{
	for (piece_t type = bishop; type <= queen; ++type) {
		bitboard_t myPieces = mask_[player_ | type];
		while (myPieces) {
			square_t from = Magic::extractBit(myPieces);
			bitboard_t moves = 0L;
			if (type & 1) moves |= Bmagic(from, occupied_);
			if (type & 2) moves |= Rmagic(from, occupied_);
			moves &= ~mask_[player_];
			moves &= allowed;
			while (moves) {
				int to = Magic::extractBit(moves);
				movelist.push_back(MAKE_MOVE_FT(from, to));
			}
		}
	}
}

void ChessBoard::generateMovesPawn(std::vector<move_t>& movelist, bool captures_only) const
{
	bitboard_t myPawns = mask_[player_ | pawn];
	// Direction is +8 / -8 for white player / black player
	int direction = 8 - (player_ << 1);
	
	if (!captures_only) {
		// Single step forward
		bitboard_t step1 = (player_ == white ? myPawns << 8 : myPawns >> 8) & (~occupied_);
		bitboard_t step2 = step1;
		while (step1) {
			square_t to = Magic::extractBit(step1);
			move_t move = (to - direction) | (to << 6);
			if (to >= Data::square_a8 || to <= Data::square_h1) {
				movelist.push_back(move | (Data::move_promotion_queen << 12));
				movelist.push_back(move | (Data::move_promotion_rook << 12));
				movelist.push_back(move | (Data::move_promotion_bishop << 12));
				movelist.push_back(move | (Data::move_promotion_knight << 12));
			} else {
				movelist.push_back(move);
			}
		}
		// Double step forward
		step2 = (player_ == white ? step2 << 8 : step2 >> 8) & (~occupied_);
		step2 &= Data::line[3 + (player_ >> 3)];
		while (step2) {
			square_t to = Magic::extractBit(step2);
			movelist.push_back(MAKE_MOVE_FTS(to - 2*direction, to, Data::move_double_pawn_push));
		}
	}
	
	// Capture to the left (towards the a-file)
	bitboard_t step = (player_ == white ? myPawns << 7 : myPawns >> 9) & (~Data::file[7]);
	step &= mask_[player_ ^ opponent];
	while (step) {
		square_t to = Magic::extractBit(step);
		move_t move = (to - direction + 1) | (to << 6);
		if (to >= Data::square_a8 || to <= Data::square_h1) {
			movelist.push_back(move | (Data::move_promotion_queen << 12));
			movelist.push_back(move | (Data::move_promotion_rook << 12));
			movelist.push_back(move | (Data::move_promotion_bishop << 12));
			movelist.push_back(move | (Data::move_promotion_knight << 12));
		} else {
			movelist.push_back(move);
		}
	}
	
	// Capture to the right (towards the h-file)
	step = (player_ == white ? myPawns << 9 : myPawns >> 7) & (~Data::file[0]);
	step &= mask_[player_ ^ opponent];
	while (step) {
		square_t to = Magic::extractBit(step);
		move_t move = (to - direction - 1) | (to << 6);
		if (to >= Data::square_a8 || to <= Data::square_h1) {
			movelist.push_back(move | (Data::move_promotion_queen << 12));
			movelist.push_back(move | (Data::move_promotion_rook << 12));
			movelist.push_back(move | (Data::move_promotion_bishop << 12));
			movelist.push_back(move | (Data::move_promotion_knight << 12));
		} else {
			movelist.push_back(move);
		}
	}
	
	// En passant capture
	if (enpassant_ > 0) {
		square_t to = enpassant_ + direction;
		square_t from1 = enpassant_ + 1;
		square_t from2 = enpassant_ - 1;
		if ((from1 % 8) != 0 && board_[from1] == (player_ | pawn)) {
			movelist.push_back(MAKE_MOVE_FTS(from1, to, Data::move_enpassant_capture));
		}
		if ((from2 % 8) != 7 && board_[from2] == (player_ | pawn)) {
			movelist.push_back(MAKE_MOVE_FTS(from2, to, Data::move_enpassant_capture));
		}
	}
}

// Note: Castling INTO check is allowed here, because leaving the king in check
//       after a move is illegal and will be caught elsewhere anyway.
void ChessBoard::generateCastles(std::vector<move_t>& movelist) const
{
	if (player_ == white) {
		int e1_attacked = -1;
		if (castling_ & Data::castle_white_kingside) {
			if ((occupied_ & Data::castle_squares_white_kingside) == 0) {
				e1_attacked = isSquareAttacked(Data::square_e1, black);
				if (!e1_attacked && !isSquareAttacked(Data::square_f1, black)) {
					movelist.push_back(Data::fullmove_castle_white_k);
				}
			}
		}
		if (castling_ & Data::castle_white_queenside) {
			if ((occupied_ & Data::castle_squares_white_queenside) == 0) {
				if (e1_attacked < 0) e1_attacked = isSquareAttacked(Data::square_e1, black);
				if (!e1_attacked && !isSquareAttacked(Data::square_d1, black)) {
					movelist.push_back(Data::fullmove_castle_white_q);
				}
			}
		}
	} else {
		int e8_attacked = -1;
		if (castling_ & Data::castle_black_kingside) {
			if ((occupied_ & Data::castle_squares_black_kingside) == 0) {
				e8_attacked = isSquareAttacked(Data::square_e8, white);
				if (!e8_attacked && !isSquareAttacked(Data::square_f8, white)) {
					movelist.push_back(Data::fullmove_castle_black_k);
				}
			}
		}
		if (castling_ & Data::castle_black_queenside) {
			if ((occupied_ & Data::castle_squares_black_queenside) == 0) {
				if (e8_attacked < 0) e8_attacked = isSquareAttacked(Data::square_e8, white);
				if (!e8_attacked && !isSquareAttacked(Data::square_d8, white)) {
					movelist.push_back(Data::fullmove_castle_black_q);
				}
			}
		}
	}
}

int ChessBoard::isSquareAttacked(square_t square, player_t color) const
{
	// Attacked by bishop- or rook-like pieces?
	if (Bmagic(square, occupied_ | BIT(square)) & (mask_[color|bishop] | mask_[color|queen])) return 1;
	if (Rmagic(square, occupied_ | BIT(square)) & (mask_[color|rook] | mask_[color|queen])) return 1;
	
	// Attacked by knights or king?
	if (Data::attacks_knight[square] & mask_[color|knight]) return 1;
	if (Data::attacks_king[square] & mask_[color|king]) return 1;
	
	// Attacked by pawn?
	if (color == white) {
		if (mask_[white|pawn] & (BIT(square-7) | BIT(square-9))) return 1;
	} else {
		if (mask_[black|pawn] & (BIT(square+7) | BIT(square+9))) return 1;
	}
	
	return 0;
}

bool ChessBoard::isKingAttacked(player_t color) const
{
	square_t myKing = Magic::firstBit(mask_[color|king]);
	return isSquareAttacked(myKing, color ^ opponent);
}

void ChessBoard::sortMoves(std::vector<move_t>& movelist, move_t sortFirst) const
{
	for (size_t i = 0; i < movelist.size(); ++i) {
		move_t& move = movelist[i];
		if (move == sortFirst) {
			// Guarantee the first position for the specified move
			move |= ((u16)Data::priorityMax) << 16;
		} else {
			// Compute the priority of the move
			piece_t piece = board_[MOVE_FROM(move)] & mask_piecetype;
			piece_t capture = board_[MOVE_TO(move)] & mask_piecetype;
			int priority = Data::priorityCaptures[piece][capture] + Data::prioritySpecial[MOVE_SPECIAL(move)];
			move |= ((u16)priority) << 16;
		}
	}
	std::sort(movelist.begin(), movelist.end(), std::greater<move_t>());
}

void ChessBoard::doMove(move_t move)
{
	// Move format:
	// 6 bit = from (shift 0)
	// 6 bit = to (shift 6)
	// 4 bit = special move
	
	/*if (!isValidMove(move)) {
		// TODO: tie to debug build only
		std::cerr << "ERROR: Invalid move:\n";
		printBoard(std::cerr);
		printMove(std::cerr, move);
		return;
	}*/
	
	// Read move data
	square_t from = MOVE_FROM(move);
	square_t to = MOVE_TO(move);
	u16 special = MOVE_SPECIAL(move);
	piece_t arrive = board_[from];
	
	// Save history
	HistoryInfo history;
	history.capture = board_[to];
	history.castling = castling_;
	history.enpassant = enpassant_;
	history.drawmoves = drawmoves_;
	history.zobrist = zobrist_;
	history_.push(history);
	
	// Update move numbers
	if (board_[to] != nothing || (board_[from] & mask_piecetype) == pawn) {
		drawmoves_ = 0;
	} else {
		++drawmoves_;
	}
	if (player_ == black) ++movenumber_;
	
	// Apply special move rules:
	// Castling, pawn promotions, en passant capturing, double pawn push
	switch_zobrist(Data::zobrist_enpassant | enpassant_);
	enpassant_ = 0;
	
	switch (special) {
	case Data::move_castle_kingside:
		remove_piece(Data::square_h1 + player_ * 7);
		insert_piece(Data::square_f1 + player_ * 7, player_ | rook);
		break;
	case Data::move_castle_queenside:
		remove_piece(Data::square_a1 + player_ * 7);
		insert_piece(Data::square_d1 + player_ * 7, player_ | rook);
		break;
	case Data::move_promotion_knight:
		arrive = player_ | knight;
		break;
	case Data::move_promotion_bishop:
		arrive = player_ | bishop;
		break;
	case Data::move_promotion_rook:
		arrive = player_ | rook;
		break;
	case Data::move_promotion_queen:
		arrive = player_ | queen;
		break;
	case Data::move_enpassant_capture:
		remove_piece(history.enpassant);
		break;
	case Data::move_double_pawn_push:
		enpassant_ = to;
		switch_zobrist(Data::zobrist_enpassant | enpassant_);
		break;
	}
	
	// Move piece (and capture)
	remove_piece(from);
	remove_piece(to);
	insert_piece(to, arrive);
	
	// Update castling status
	switch_zobrist(Data::zobrist_castling | castling_);
	castling_ &= Data::update_castling[from];
	castling_ &= Data::update_castling[to];
	switch_zobrist(Data::zobrist_castling | castling_);
	
	// Switch player
	player_ ^= opponent;
	switch_zobrist(Data::zobrist_player);
}

void ChessBoard::undoMove(move_t move)
{
	// Get move data
	square_t from = MOVE_FROM(move);
	square_t to = MOVE_TO(move);
	u16 special = MOVE_SPECIAL(move);
	piece_t leave = board_[to];
	HistoryInfo history = history_.top();
	history_.pop();
	
	// Switch player
	player_ ^= opponent;
	
	// Apply special move rules:
	// Castling, pawn promotions, en passant capturing
	switch (special) {
	case Data::move_castle_kingside:
		remove_piece(Data::square_f1 + player_ * 7);
		insert_piece(Data::square_h1 + player_ * 7, player_ | rook);
		break;
	case Data::move_castle_queenside:
		remove_piece(Data::square_d1 + player_ * 7);
		insert_piece(Data::square_a1 + player_ * 7, player_ | rook);
		break;
	case Data::move_promotion_knight:
	case Data::move_promotion_bishop:
	case Data::move_promotion_rook:
	case Data::move_promotion_queen:
		leave = player_ | pawn;
		break;
	case Data::move_enpassant_capture:
		insert_piece(history.enpassant, (player_ ^ opponent) | pawn);
		break;
	}
	
	// Move piece (and insert captured piece)
	remove_piece(to);
	if (history.capture != nothing) insert_piece(to, history.capture);
	insert_piece(from, leave);
	
	// Reload values from history
	if (player_ == black) --movenumber_;
	castling_ = history.castling;
	enpassant_ = history.enpassant;
	drawmoves_ = history.drawmoves;
	zobrist_ = history.zobrist;
}

bool ChessBoard::leavesKingInCheck(move_t move)
{
	// Read data
	square_t from = MOVE_FROM(move);
	square_t to = MOVE_TO(move);
	u8 special = MOVE_SPECIAL(move);
	piece_t capture = board_[to];
	bitboard_t undo_occupied(occupied_);
	
	// Update only the relevant masks
	// Attacking piece may have been captured or own piece moved in between
	occupied_ ^= BIT(from);
	occupied_ |= BIT(to);
	if (capture != nothing) {
		mask_[capture] ^= BIT(to);
	} else if (special == Data::move_enpassant_capture) {
		occupied_ ^= BIT(enpassant_);
		mask_[(player_ ^ opponent) | pawn] ^= BIT(enpassant_);
	}
	
	// Check if the king is attacked by the opponent
	// Note: King might have been the moving piece to escape check
	square_t myKing = to;
	if ((board_[from] & mask_piecetype) != king) {
		myKing = Magic::firstBit(mask_[player_ | king]);
	}
	bool inCheck = isSquareAttacked(myKing, player_ ^ opponent);
	
	// Undo all changes
	occupied_ = undo_occupied;
	if (capture != nothing) {
		mask_[capture] ^= BIT(to);
	} else if (special == Data::move_enpassant_capture) {
		mask_[(player_ ^ opponent) | pawn] ^= BIT(enpassant_);
	}
	
	return inCheck;
}

void ChessBoard::rebuildZobrist()
{
	zobrist_ = 0;
	for (int i=0; i<64; ++i) {
		zobrist_ ^= Data::zobrist[i | (board_[i] << 6)];
	}
	zobrist_ ^= Data::zobrist[Data::zobrist_castling | castling_];
	zobrist_ ^= Data::zobrist[Data::zobrist_enpassant | enpassant_];
	if (player_ == black) zobrist_ ^= Data::zobrist[Data::zobrist_player];
}

bool ChessBoard::setPosition(const tokenizer& tokens, tokenizer::iterator& token)
{
	if (token == tokens.end()) return false;
	
	if (*token == "fen") {
		++token;
		if (token == tokens.end()) return false;
	}
	
	if (*token == "startpos") {
		setInitialPosition();
		++token;
	} else {
		std::stringstream ss;
		
		// Parse FEN: RNBQKBNR/PPPPPPPP/8/8/8/8/pppppppp/rnbqkbnr
		if (!parseFEN(*token)) return false;
		++token;
		
		// Parse player: w OR b
		if (token == tokens.end()) return false;
		if (!parsePlayer(*token)) return false;
		++token;
		
		// Parse castling flags: QKqk OR -
		if (token == tokens.end()) return false;
		if (!parseCastling(*token)) return false;
		++token;
		
		// Parse en passant state:
		if (token == tokens.end()) return false;
		if (!parseEnpassant(*token)) return false;
		++token;
		
		// Parse move number: 21
		// Note: We need to read to a temporary int because drawmoves_ is a char
		if (token == tokens.end()) return false;
		ss = std::stringstream(*token);
		int integerDrawmoves;
		ss >> integerDrawmoves;
		drawmoves_ = (u8)integerDrawmoves;
		++token;
		
		// Parse draw moves: 7
		if (token == tokens.end()) return false;
		ss = std::stringstream(*token);
		ss >> movenumber_;
		++token;
		
		// Zobrist Hash for this position
		rebuildZobrist();
	}
	
	if (token == tokens.end() || *token != "moves") return true;
	++token;
	
	while (token != tokens.end()) {
		move_t move = parseMove(*token);
		if (!isValidMove(move)) return false;
		doMove(move);
		++token;
	}
	
	return true;
}

void ChessBoard::setInitialPosition()
{
	parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
	player_ = white;
	castling_ = 0xF;
	enpassant_ = 0;
	drawmoves_ = 0;
	movenumber_ = 1;
	rebuildZobrist();
}

// Fills occupied, whites, blacks and board with data from the FEN-string
bool ChessBoard::parseFEN(const string& fen)
{
	std::fill_n(mask_, 16, 0L);
	std::fill_n(board_, 64, 0);
	
	square_t square = 56; // upper left corner
	piece_t piece = nothing;
	for (char c : fen) {
		switch (c) {
			case 'P': piece = pawn | white; break;
			case 'N': piece = knight | white; break;
			case 'B': piece = bishop | white; break;
			case 'R': piece = rook | white; break;
			case 'Q': piece = queen | white; break;
			case 'K': piece = king | white; break;
			case 'p': piece = pawn | black; break;
			case 'n': piece = knight | black; break;
			case 'b': piece = bishop | black; break;
			case 'r': piece = rook | black; break;
			case 'q': piece = queen | black; break;
			case 'k': piece = king | black; break;
			case '/':
			case '\\':
				square -= 16;
				piece = nothing;
				break;
			default:
				if (c < '1' || c > '8') return false;
				square += (c - '0');
				piece = nothing;
				break;
		}
		if (piece != nothing) {
			mask_[piece] |= BIT(square);
			board_[square] = piece;
			++square;
		}
	}
	for (int i=1; i<8; ++i) {
		mask_[white] |= mask_[white | i];
		mask_[black] |= mask_[black | i];
	}
	occupied_ = mask_[white] | mask_[black];
	
	return true;
}

bool ChessBoard::parsePlayer(const string& player)
{
	if (player == "w") {
		player_ = white;
	} else if (player == "b") {
		player_ = black;
	} else {
		return false;
	}
	return true;
}

bool ChessBoard::parseCastling(const string& castling)
{
	castling_ = 0;
	if (castling == "-") return true;
	
	for (char c : castling) {
		if (c == 'K') {
			castling_ |= Data::castle_white_kingside;
		} else if (c == 'k') {
			castling_ |= Data::castle_black_kingside;
		} else if (c == 'Q') {
			castling_ |= Data::castle_white_queenside;
		} else if (c == 'q') {
			castling_ |= Data::castle_black_queenside;
		} else {
			return false;
		}
	}
	return true;
}

bool ChessBoard::parseEnpassant(const string& enpassantSquare)
{
	if (enpassantSquare == "-") {
		enpassant_ = 0;
		return true;
	}
	if (enpassantSquare.length() != 2) return false;
	enpassant_ = parseSquare(enpassantSquare);
	if (enpassant_ >= 64) return false;
	return true;
}

square_t ChessBoard::parseSquare(const string& squareStr)
{
	assert(squareStr.length() == 2);
	square_t square = (squareStr[1] - '1') * 8 + (squareStr[0] - 'a');
	return square;
}

string ChessBoard::nameSquare(square_t square)
{
	string result;
	result.push_back('a' + (square % 8));
	result.push_back('1' + (square / 8));
	return result;
}

// Formats: c7-c8q, e1-e2, f3xg4, e2e4, c7c8q
move_t ChessBoard::parseMove(std::string move) const
{
	static const char promote[5] = "nbrq";
	static boost::regex format("^[a-h]{1}[1-8]{1}[a-h]{1}[1-8]{1}[qrbn]?$");
	
	square_t from, to;
	u16 special = 0;
	
	// Normalize
	boost::to_lower(move);
	if (move[2] == '-' || move[2] == 'x') move = move.substr(0, 2) + move.substr(3);
	
	// Parse values
	if (boost::regex_match(move, format)) {
		from = parseSquare(move.substr(0, 2));
		to = parseSquare(move.substr(2, 2));
		if (move.length() == 5) {
			special = Data::move_promotion_knight + std::distance(promote, std::find(promote, promote+4, move[4]));
			if (special > Data::move_promotion_queen) return 0;
		} else if ((board_[from] & mask_piecetype) == pawn) {
			if (board_[to] == nothing && ((from-to) % 8) != 0) {
				special = Data::move_enpassant_capture;
			} else if ((from-to) % 16 == 0) {
				special = Data::move_double_pawn_push;
			}
		}
	} else if (move == "0-0") {
		from = player_ == white ? Data::square_e1 : Data::square_e8;
		to = player_ == white ? Data::square_g1 : Data::square_g8;
		special = Data::move_castle_kingside;
	} else if (move == "0-0-0") {
		from = player_ == white ? Data::square_e1 : Data::square_e8;
		to = player_ == white ? Data::square_c1 : Data::square_c8;
		special = Data::move_castle_queenside;
	} else {
		return 0;
	}
	
	if ((board_[from] & mask_piecetype) == king) {
		if (to - from == 2) {
			special = Data::move_castle_kingside;
		} else if (from - to == 2) {
			special = Data::move_castle_queenside;
		}
	}
	
	// Basic error checking
	if (board_[from] == nothing || from == to || from >= 64 || to >= 64) return 0;
	
	// Build move
	move_t realmove = from | (to << 6) | (special << 12);
	return realmove;
}

string ChessBoard::uciMove(move_t move) const
{
	square_t from = MOVE_FROM(move);
	square_t to = MOVE_TO(move);
	u16 special = MOVE_SPECIAL(move);
	
	string result;
	result += nameSquare(from);
	result += nameSquare(to);
	
	switch (special) {
	case Data::move_promotion_knight:
	case Data::move_promotion_bishop:
	case Data::move_promotion_rook:
	case Data::move_promotion_queen:
		result.push_back("nbrq"[special - Data::move_promotion_knight]);
		break;
	}
	
	return result;
}

// Performs a quick check whether a move is valid
// Does NOT check against all possible errors
bool ChessBoard::isValidMove(move_t move) const
{
	square_t from = MOVE_FROM(move);
	square_t to = MOVE_TO(move);
	u16 special = MOVE_SPECIAL(move);

	// Check moving piece and target square
	if (board_[from] == nothing) return false;
	if ((board_[from] & mask_color) != player_) return false;
	if (board_[to] != nothing && (board_[to] & mask_color) == player_) return false;
	
	// Check special rules
	switch (special) {
	case Data::move_promotion_knight:
	case Data::move_promotion_bishop:
	case Data::move_promotion_rook:
	case Data::move_promotion_queen:
		if ((board_[from] & mask_piecetype) != pawn) return false;
		if ((player_ == white && to < Data::square_a8) || (player_ == black && to > Data::square_h1)) {
			return false;
		}
		break;
	case Data::move_enpassant_capture:
		if (enpassant_ == 0) return false;
		break;
	case Data::move_double_pawn_push:
		if ((board_[from] & mask_piecetype) != pawn) return false;
		break;
	case Data::move_castle_kingside:
	case Data::move_castle_queenside:
		if ((board_[from] & mask_piecetype) != king) return false;
		break;
	}
	
	return true;
}

void ChessBoard::printBoard(std::ostream& out) const
{
	static const string pieceToChar = ".PNK?BRQ?pnk?brq";
	for (int i=7; i>=0; --i) {
		for (int j=0; j<8; ++j) {
			out << pieceToChar[board_[i*8+j]];
		}
		out << '\n';
	}
	out << "castling: " << (int)castling_ << '\n';
	out << "enpassant: " << (int)enpassant_ << '\n';
	out << "drawmoves: " << (int)drawmoves_ << '\n';
}

void ChessBoard::printBitboard(std::ostream& out, bitboard_t bitboard)
{
	for (int i=7; i>=0; --i) {
		for (int j=0; j<8; ++j) {
			out << ((bitboard & BIT(i*8+j)) ? '#' : '.');
		}
		out << '\n';
	}
}

void ChessBoard::printMove(std::ostream& out, move_t move) const
{
	static const string pieceToChar = ".PNK?BRQ?pnk?brq";
	static const string promotions = "nbrq";
	
	square_t from = MOVE_FROM(move);
	square_t to = MOVE_TO(move);
	u16 special = MOVE_SPECIAL(move);
	out << "Move: " << pieceToChar[board_[from]] << nameSquare(from);
	out << (board_[to] == nothing ? "-" : "x") << nameSquare(to);
	switch (special) {
	case Data::move_promotion_knight:
	case Data::move_promotion_bishop:
	case Data::move_promotion_rook:
	case Data::move_promotion_queen:
		out << promotions[special - Data::move_promotion_knight];
		break;
	case Data::move_enpassant_capture:
		out << " en passant";
		break;
	case Data::move_double_pawn_push:
		out << " double push";
		break;
	case Data::move_castle_kingside:
		out << " castle kingside";
		break;
	case Data::move_castle_queenside:
		out << " castle queenside";
		break;
	}
	out << std::endl;
}

void ChessBoard::printDebug(std::ostream& out) const
{
	printBoard(out);
	out << std::endl;
	printBitboard(out, occupied_);
	out << std::endl;
	printBitboard(out, mask_[black]);
	out << std::endl;
	printBitboard(out, mask_[white | rook]);
	out << std::endl;
	out << "Castling: " << (int)castling_ << " Enpassant: " << enpassant_ << " Drawmoves: ";
	out << drawmoves_ << " Movenumber: " << movenumber_ << std::endl << std::endl;
}
