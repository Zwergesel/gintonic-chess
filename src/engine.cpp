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
	search_.depth = 1;
	search_.startTime = steady_clock::now();
	search_.maxTime = 15000;
	
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
		// Stop because maximum search time was exceeded
		auto milli = duration_cast<milliseconds>(steady_clock::now() - search_.startTime).count();
		std::cout << "Timer: " << milli << " milliseconds have passed" << std::endl;
		if (milli > search_.maxTime) break;

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
		
		std::cout << "info depth " << (int)search_.depth << " score ";
		if (abs(bestvalue) < Score::mate_bound) {
			std::cout << "cp " << bestvalue;
		} else {
			std::cout << "mate " << ((Score::checkmate - abs(bestvalue) + 1) / 2);
		}
		std::cout << " pv " << board_.uciMove(bestmove);
		for (move_t move : bestpv) std::cout << " " << board_.uciMove(move);
		std::cout << std::endl;
		++search_.depth;
	}
	
	auto milli = duration_cast<milliseconds>(steady_clock::now() - search_.startTime).count();
	std::cout << "Final Timer: " << milli << " milliseconds have passed" << std::endl;
	string pvinfo = "info pv " + board_.uciMove(bestmove);
	for (move_t move : bestpv) pvinfo += " " + board_.uciMove(move);
	UCIProtocol::sendMessage(pvinfo);
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
		score_t value = Evaluator::evaluatePosition(board_);
		return value;
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
	
	for (int i=0; i<movelist.size(); ++i)
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
	return 0;
}
