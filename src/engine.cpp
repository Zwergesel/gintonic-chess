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
	
	// Sort moves to search potentially better moves first
	board_.sortMoves(movelist);
	
	// TODO put this somewhere else??
	// We store this for when the search cancels in the middle of a new round
	score_t bestvalue = 0;
	move_t bestmove = 0;
	std::vector<move_t> bestpv;
	
	while (search_.depth <= search_.maxDepth)
	{
		// Stop because maximum search time was exceeded
		auto milli = duration_cast<milliseconds>(steady_clock::now() - search_.startTime).count();
		std::cout << "Timer: " << milli << " milliseconds have passed" << std::endl;
		if (milli > search_.maxTime) break;

		score_t alpha = -Score::infinity;
		score_t beta = Score::infinity;
		
		move_t roundmove = 0;
		std::vector<move_t> roundpv;
		
		for (int i=0; i<movelist.size(); ++i)
		{
			std::vector<move_t> localpv;
			board_.doMove(movelist[i]);
			score_t value = -NegaMax(1, -beta, -alpha, true, localpv);
			board_.undoMove(movelist[i]);
			
			if (value > alpha) {
				alpha = value;
				roundmove = movelist[i];
				roundpv.assign(localpv.begin(), localpv.end());
			}
		}
		
		// Adjust for checkmate depth
		if (Score::mate_bound <= alpha && alpha <= Score::checkmate) {
			alpha -= 1;
		} else if (-Score::mate_bound >= alpha && alpha >= -Score::checkmate) {
			alpha += 1;
		}
		
		bestvalue = alpha;
		bestmove = roundmove;
		bestpv.assign(roundpv.begin(), roundpv.end());
		
		std::cout << "info depth " << (int)search_.depth << " score " << bestvalue;
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
	
	if (depth == search_.depth) {
		score_t value = Evaluator::evaluatePosition(board_);
		return value;
	}
	
	std::vector<move_t> movelist;
	board_.generateMoves(movelist);
	
	// Checkmate and stalemate
	if (movelist.size() == 0) {
		if (board_.isKingAttacked(board_.player_)) {
			return -Score::checkmate;
		} else {
			return Score::stalemate;
		}
	}
	
	// Sort moves to search potentially better moves first
	board_.sortMoves(movelist);
	
	// Search all follow-up moves
	for (int i=0; i<movelist.size(); ++i)
	{
		std::vector<move_t> localpv;
		board_.doMove(movelist[i]);
		score_t value = -NegaMax(depth+1, -beta, -alpha, true, localpv);
		board_.undoMove(movelist[i]);
		
		if (value >= beta) {
			return beta;
		}
		if (value > alpha) {
			alpha = value;
			deeppv.clear();
			deeppv.push_back(movelist[i]);
			deeppv.insert(deeppv.end(), localpv.begin(), localpv.end());
		}
	}
	
	// Adjust for checkmate depth
	if (Score::mate_bound <= alpha && alpha <= Score::checkmate) {
		alpha -= 1;
	} else if (-Score::mate_bound >= alpha && alpha >= -Score::checkmate) {
		alpha += 1;
	}
	return alpha;
}

score_t Engine::QuiescenceSearch(int depth, score_t alpha, score_t beta)
{
	++info_.nodesSearched;
	if (depth > info_.selectiveDepthReached) info_.selectiveDepthReached = depth;
	
	// Stop command received?
	if (think_ == thinkStop) return Score::command_stop;
	
	score_t value = Score::unknown;
	//= hashtable_.probeHash(board_.zobrist_, search_.depth + depth, 0, alpha, beta);
	if (value == Score::unknown) {
		value = Evaluator::evaluatePosition(board_);
	}
	if (value >= beta) return beta;
	if (value > alpha) alpha = value;
	
	std::vector<move_t> movelist;
	board_.generateGoodCaptures(movelist);
	if (movelist.size() == 0) return Score::quiescence_check;
	
	/*
	compareHash = 0;
	comparePly = Const.MAX_SEARCH - 1;
	sortMoves(move_list, 0, move_list.size() - 1);
	*/
	
	for (int i=0; i<movelist.size(); ++i) {
		board_.doMove(movelist[i]);
		value = -QuiescenceSearch(depth+1, -beta, -alpha);
		board_.undoMove(movelist[i]);
		
		if (value >= beta) return beta;
		if (value > alpha) alpha = value;
	}
	
	return alpha;
}
