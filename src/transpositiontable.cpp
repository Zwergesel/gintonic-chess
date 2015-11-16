#include <iostream>

#include "data.hpp"
#include "score.hpp"
#include "transpositiontable.hpp"

#define AGE_DECAY 8

TranspositionTable::TranspositionTable(int maxSize)
{
	sizeMask_ = 1ULL << 32;
	while (sizeMask_ * sizeof(HashEntry) > maxSize) sizeMask_ >>= 1;
	table_.resize(sizeMask_);
	std::cout << "Hash table initialized: " << sizeMask_ << " entries, ";
	std::cout << ((sizeMask_ * sizeof(HashEntry)) / 1024) << "kb total size" << std::endl;
	--sizeMask_;
}

void TranspositionTable::recordHash(u64 zobrist,
									score_t value,
									hashf type,
									int depth,
									int age,
									move_t move)
{
	HashEntry& entry = table_[zobrist & sizeMask_];
	if (depth >= entry.depth || depth + age >= entry.depth + entry.age + AGE_DECAY) {
		entry.zobrist = zobrist;
		entry.value = (u16) value;
		entry.move = (u16) move;
		entry.depth = (u8) depth;
		entry.type = (u8) type;
	}
}

int TranspositionTable::probeHash(u64 zobrist,
								  int depth,
								  score_t alpha,
								  score_t beta,
								  move_t& outMove) const
{
	// TODO: return hashf type???
	const HashEntry& entry = table_[zobrist & sizeMask_];
	if (entry.zobrist == zobrist) {
		outMove = entry.move;
		if (entry.depth >= depth) {
			score_t value = entry.value;
			if (entry.type == hashfExact) {
				return value;
			} else if (entry.type == hashfAlpha && value <= alpha) {
				return alpha;
			} else if (entry.type == hashfBeta && value >= beta) {
				return beta;
			}
		} else {
			// TODO: notify that this move should be searched first
		}
	} else {
		outMove = 0;
	}
	return Score::unknown;
}

move_t TranspositionTable::getMove(u64 zobrist) const
{
	const HashEntry& entry = table_[zobrist & sizeMask_];
	if (entry.zobrist == zobrist) {
		return entry.move;
	} else {
		return 0;
	}
}
