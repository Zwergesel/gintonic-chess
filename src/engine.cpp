#include <sstream>

#include "engine.hpp"
#include "evaluator.hpp"
#include "score.hpp"

#define ASPIRATION_WINDOW 100
#define NULL_MOVE_PRUNING 2

using std::string;
using namespace std::chrono;

Engine::Engine():hashtable_(64*1024*1024)
{
}

void Engine::Search()
{
	info_.nodesSearched = 0;
	info_.selectiveDepthReached = 0;
	
	search_.maxDepth = 15;
	search_.quiescenceDepth = 8;
	search_.depth = 1;
	search_.startTime = steady_clock::now();
	search_.maxTime = 8000;
	
	think_ = thinkSearch;
	
	std::vector<move_t> movelist;
	board_.generateMoves(movelist);
	
	if (movelist.size() == 0) {
		// No move available: Position is checkmate or stalemate
		UCIProtocol::sendMessage("bestmove 0000");
		return;
	} else if (movelist.size() == 1) {
		// Only one move available: Make it!
		UCIProtocol::sendMessage("bestmove "+board_.uciMove(movelist[0]));
		return;
	}
	
	// TODO put this somewhere else??
	// We store this for when the search cancels in the middle of a new round
	score_t bestvalue = 0;
	move_t bestmove = 0;
	std::vector<move_t> bestpv;
	
	while (search_.depth <= search_.maxDepth && abs(bestvalue) < Score::mate_bound)
	{
		// Sort move list; Top half will contain value from previous round
		std::sort(movelist.begin(), movelist.end(), std::greater<move_t>());
		
		score_t alpha = -Score::infinity;
		score_t beta = Score::infinity;
		
		move_t roundmove = 0;
		std::vector<move_t> roundpv;
		
		for (int i=0; i<movelist.size(); ++i)
		{
			std::vector<move_t> localpv;
			board_.doMove(movelist[i]);
			score_t value = -NegaMax(search_.depth - 1, -beta, -alpha, true, localpv);
			board_.undoMove(movelist[i]);
			
			u32 wide_value = (32768 + value) << 16;
			movelist[i] &= 0xffff;
			movelist[i] |= wide_value;
			
			if (value > alpha) {
				alpha = value;
				roundmove = movelist[i];
				roundpv.assign(localpv.begin(), localpv.end());
			}
		}
		
		bestvalue = alpha;
		bestmove = roundmove;
		bestpv.assign(roundpv.begin(), roundpv.end());
		
		// Display search information
		auto milli = duration_cast<milliseconds>(steady_clock::now() - search_.startTime).count();
		auto nps = (info_.nodesSearched / std::max(1LL, (milli / 1000)));
		std::stringstream ss;
		ss << "info depth " << search_.depth << " seldepth " << info_.selectiveDepthReached;
		ss << " nodes " << info_.nodesSearched << " nps " << nps << " score ";
		if (abs(bestvalue) < Score::mate_bound) {
			ss << "cp " << bestvalue;
		} else {
			ss << "mate " << ((Score::checkmate - abs(bestvalue) + 1) / 2);
		}
		ss << " pv " << board_.uciMove(bestmove);
		for (move_t move : bestpv) ss << " " << board_.uciMove(move);
		UCIProtocol::sendMessage(ss.str());
		++search_.depth;
		
		// Stop because maximum search time was exceeded
		if (milli > search_.maxTime) break;
	}
	
	UCIProtocol::sendMessage("bestmove " + board_.uciMove(bestmove));
	think_ = thinkStop;
}

score_t Engine::NegaMax(int depth, score_t alpha, score_t beta, bool nullmove, std::vector<move_t>& deeppv)
{
	++info_.nodesSearched;
	move_t bestMove = 0;
	
	// Draw by 50-move rule (= 100 half-moves)
	if (board_.drawmoves_ == 100) return Score::stalemate;
	
	// Query hashtable for previous results
	const auto& entry = hashtable_.getEntry(board_.zobrist_);
	if (entry.zobrist == board_.zobrist_) {
		// Hash entry is deep enough to be used directly?
		if (entry.depth >= depth) {
			score_t value = entry.value;
			if (value > Score::mate_bound) {
				value -= search_.depth - depth;
			} else if (value < -Score::mate_bound) {
				value += search_.depth - depth;
			}
			if (entry.type == TranspositionTable::hashfExact) {
				return value;
			} else if (entry.type == TranspositionTable::hashfBeta) {
				alpha = std::max(alpha, value);
			} else if (entry.type == TranspositionTable::hashfAlpha) {
				beta = std::min(beta, value);
			}
			if (alpha >= beta) return value;
		}
		// Otherwise just use the previously best move as the first one for searching
		bestMove = entry.move;
	}
	
	// Reached a leaf of the search. Evaluate the position.
	if (depth == 0) {
		if (board_.lastMoveWasQuiet()) {
			return Evaluator::evaluatePosition(board_);
		} else {
			--info_.nodesSearched;
			return QuiescenceSearch(search_.quiescenceDepth, alpha, beta);
		}
	}
	
	// Generate all moves from this position
	std::vector<move_t> movelist;
	board_.generateMoves(movelist);
	
	// Catch checkmate and stalemate
	if (movelist.size() == 0) {
		if (board_.isKingAttacked(board_.player_)) {
			// Checkmate: Subtract depth to score faster mates higher
			return -(Score::checkmate - (search_.depth - depth));
		} else {
			return Score::stalemate;
		}
	}
	
	// Sort moves to search potentially better moves first
	board_.sortMoves(movelist, bestMove);
	
	// Search all follow-up moves
	TranspositionTable::HashType hashType = TranspositionTable::hashfAlpha;
	score_t gamma = -Score::infinity;
	
	for (unsigned i=0; i<movelist.size(); ++i)
	{
		std::vector<move_t> localpv;
		board_.doMove(movelist[i]);
		score_t value = -NegaMax(depth-1, -beta, -alpha, true, localpv);
		board_.undoMove(movelist[i]);
		
		// Gamma is the best score from this position
		if (value > gamma) {
			gamma = value;
			bestMove = (u16)movelist[i];
			deeppv.clear();
			deeppv.push_back(bestMove);
			deeppv.insert(deeppv.end(), localpv.begin(), localpv.end());
		}
		// Alpha is our best score so far
		if (value > alpha) {
			alpha = value;
			hashType = TranspositionTable::hashfExact;
		}
		// Beta is the worst score the opponent can force on us
		// It causes cutoffs when we exceed it because the opponent will not play this line
		if (alpha >= beta) {
			hashType = TranspositionTable::hashfBeta;
			break;
		}
	}
	
	if (gamma > Score::mate_bound) {
		gamma += search_.depth - depth;
	} else if (gamma < -Score::mate_bound) {
		gamma -= search_.depth - depth;
	}
	hashtable_.recordHash(board_.zobrist_, gamma, hashType, depth, board_.movenumber_, bestMove);
	
	return alpha;
}

score_t Engine::QuiescenceSearch(int depth, score_t alpha, score_t beta)
{
	int selectiveDepth = search_.depth + search_.quiescenceDepth - depth;
	info_.selectiveDepthReached = std::max(info_.selectiveDepthReached, selectiveDepth);
	++info_.nodesSearched;
	
	score_t stand_pat = Evaluator::evaluatePosition(board_);
	if (stand_pat >= beta || depth == 0) return stand_pat;
	if (alpha < stand_pat) alpha = stand_pat;
	
	std::vector<move_t> movelist;
	board_.generateGoodCaptures(movelist);
	board_.sortMoves(movelist, 0);
	
	for (move_t move : movelist) {
		board_.doMove(move);
		score_t value = -QuiescenceSearch(depth-1, -beta, -alpha);
		board_.undoMove(move);
		
		if (value > beta) return beta;
		if (value > alpha) alpha = value;
	}
	
	return alpha;
}
