#pragma once

#include <vector>
#include "types.hpp"

class TranspositionTable
{
public:

	enum hashf {
		hashfEmpty, hashfExact, hashfAlpha, hashfBeta
	};
	
	TranspositionTable(int maxSize);
	void recordHash(u64 zobrist, score_t value, hashf type, int depth, int age, move_t move);
	int probeHash(u64 zobrist, int depth, score_t alpha, score_t beta, move_t& outMove) const;
	move_t getMove(u64 zobrist) const;
	
private:
	
	#pragma pack(push, 1)
	struct HashEntry {
		u64 zobrist;
		u16 value;
		u16 move;
		u8 depth;
		u8 type;
		u8 age;
	};
	#pragma pack(pop)
	
	size_t sizeMask_;
	std::vector<HashEntry> table_;
	
};
