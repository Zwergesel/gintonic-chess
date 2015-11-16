#pragma once

#include "types.hpp"

// Forward declarations
class ChessBoard;

class Evaluator
{
public:
	static score_t evaluatePosition(const ChessBoard& board);
	
private:
	
};