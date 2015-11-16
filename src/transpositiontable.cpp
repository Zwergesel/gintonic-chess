#include "data.hpp"
#include "score.hpp"
#include "transpositiontable.hpp"

TranspositionTable::TranspositionTable(int maxSize)
{
	sizeMask_ = 1ULL << 32;
	while (sizeMask_ * sizeof(HashEntry) > maxSize) sizeMask_ >>= 1;
	table_.resize(sizeMask_);
	--sizeMask_;
}

void TranspositionTable::recordHash(u64 zobrist,
									int searchDepth,
									int depth,
									score_t value,
									hashf flag,
									move_t move)
{
	size_t index = zobrist & sizeMask_;
	if (value > Score::mate_bound) {
		value += searchDepth - depth;
	} else if (value < -Score::mate_bound) {
		value -= searchDepth - depth;
	}
	if (depth >= table_[index].depth) {
		table_[index].zobrist = zobrist;
		table_[index].value = (u16) value;
		table_[index].depth = (u8) depth;
		table_[index].flag = (u8) flag;
		table_[index].move = (u16) move;
	}
}

int TranspositionTable::probeHash(u64 zobrist,
								  int searchDepth,
								  int depth,
								  score_t alpha,
								  score_t beta) const
{
	size_t index = zobrist & sizeMask_;
	const HashEntry& entry = table_[index];
	if (entry.zobrist == zobrist) {
		if (entry.depth >= depth) {
			score_t value = entry.value;
			if (value > Score::mate_bound) {
				value -= searchDepth - depth;
			} else if (value < -Score::mate_bound) {
				value += searchDepth - depth;
			}
			if (entry.flag == hashfExact) {
				return value;
			} else if (entry.flag == hashfAlpha && value <= alpha) {
				return alpha;
			} else if (entry.flag == hashfBeta && value >= beta) {
				return beta;
			}
		}
	}
	return Score::unknown;
}

move_t TranspositionTable::getMove(u64 zobrist) const
{
	size_t index = zobrist & sizeMask_;
	if (table_[index].zobrist == zobrist) {
		return table_[index].move;
	} else {
		return 0;
	}
}
