#pragma once

#include <chrono>
#include <string>
#include <vector>
#include "chessboard.hpp"
#include "transpositiontable.hpp"
#include "uci.hpp"

class Engine
{
public:
	enum ThinkMode
	{
		thinkStop = 0,
		thinkSearch = 1,
		thinkPonder = 2,
		thinkRefute = 3
	};
	
	Engine();
	
	// Engine info
	std::string name() { return "GinTonic Evolved v0.1"; }
	std::string author() { return "Alexander Wirth"; }
	std::vector<std::string> options() { return std::vector<std::string>(); }
	
	// Note: Race conditions shouldn't matter here
	bool isDebug() { return debug_; }
	void setDebug(bool value) { debug_ = value; }
	
	// Searching
	void Search();
	
	ChessBoard& board() { return board_; }
	
private:
	score_t NegaMax(int depth, score_t alpha, score_t beta, bool nullmove, std::vector<move_t>& deeppv);
	score_t QuiescenceSearch(int depth, score_t alpha, score_t beta);
	
	ChessBoard board_;
	TranspositionTable hashtable_;
	ThinkMode think_ = thinkStop;
	bool debug_ = false;
	
	struct SearchParameters {
		std::chrono::steady_clock::time_point startTime;
		int maxTime;
		int depth;
		int maxDepth;
		int quiescenceDepth;
	} search_;
	
	struct SearchInfo {
		u32 nodesSearched;
		int selectiveDepthReached;
	} info_;
	
};