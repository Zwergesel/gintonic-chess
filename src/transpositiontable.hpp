#pragma once

#include <vector>
#include "types.hpp"

class TranspositionTable
{
public:

	#pragma pack(push, 1)
	struct HashEntry {
		u64 zobrist;
		score_t value;
		u16 move;
		u8 depth;
		u8 type;
		u8 age;
	};
	#pragma pack(pop)
	
	enum HashType {
		hashfEmpty, hashfExact, hashfAlpha, hashfBeta
	};
	
	TranspositionTable(int maxSize);
	void recordHash(u64 zobrist, score_t value, HashType type, int depth, int age, move_t move);
	const HashEntry& getEntry(u64 zobrist) const;
	void clear();
	
private:
	
	size_t sizeMask_;
	std::vector<HashEntry> table_;
	
};
