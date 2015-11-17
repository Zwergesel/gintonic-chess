#include <iostream>

#include "data.hpp"
#include "score.hpp"
#include "transpositiontable.hpp"

#define AGE_DECAY 8

TranspositionTable::TranspositionTable(int maxSize)
{
	sizeMask_ = 1ULL << 32;
	while (sizeMask_ * sizeof(HashEntry) > maxSize) sizeMask_ >>= 1;
	
	HashEntry empty;
	empty.type = hashfEmpty;
	table_.resize(sizeMask_, empty);
	
	std::cout << "Hash table initialized: " << sizeMask_ << " entries, ";
	std::cout << ((sizeMask_ * sizeof(HashEntry)) / 1024) << "kb total size" << std::endl;
	--sizeMask_;
}

void TranspositionTable::clear()
{
	for (HashEntry& entry : table_) entry.type = hashfEmpty;
}

void TranspositionTable::recordHash(u64 zobrist,
									score_t value,
									HashType type,
									int depth,
									int age,
									move_t move)
{
	HashEntry& entry = table_[zobrist & sizeMask_];
	if (entry.type == hashfEmpty ||
		depth >= entry.depth ||
		depth + age >= entry.depth + entry.age + AGE_DECAY
	) {
		entry.zobrist = zobrist;
		entry.value = (u16) value;
		entry.move = (u16) move;
		entry.depth = (u8) depth;
		entry.type = (u8) type;
	}
}

const TranspositionTable::HashEntry& TranspositionTable::getEntry(u64 zobrist) const
{
	return table_[zobrist & sizeMask_];
}
