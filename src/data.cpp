#include "data.hpp"
#include "random.hpp"

u64 Data::zobrist[1024];
bitboard_t Data::attacks_king[64];
bitboard_t Data::attacks_knight[64];

void Data::initialize()
{
	// === ZOBRIST HASHES === //
	// zobrist 0000XXXXXX := 0L to have no effect on empty squares
	for (int i=0; i<1024; ++i) {
		if ((i & 0x3c0) == 0) {
			zobrist[i] = 0L;
		} else {
			zobrist[i] = Random::Int64();
		}
	}
	zobrist[zobrist_enpassant] = 0L;
	
	// === KING ATTACKS === //
	int moveKing[] = { -9, -8, -7, -1, 1, 7, 8, 9 };
	for (int i=0; i<64; i++) {
		attacks_king[i] = 0L;
		for (int j=0; j<8; j++) {
			if (i+moveKing[j] >= 0 && i+moveKing[j] < 64) {
				attacks_king[i] |= BIT(i+moveKing[j]);
			}
		}
		if (i%8 == 0) {
			attacks_king[i] &= ~file[0];
		} else if (i%8 == 7) {
			attacks_king[i] &= ~file[7];
		}
	}
	
	// === KNIGHT ATTACKS === //
	int moveKnight[] = { -17, -15, -10, -6, 6, 10, 15, 17 };
	for (int i=0; i<=63; i++) {
		attacks_knight[i] = 0L;
		for (int j=0; j<8; j++) {
			if (i+moveKnight[j] >= 0 && i+moveKnight[j] < 64) {
				attacks_knight[i] |= (1l << (i+moveKnight[j]));
			}
		}
		if (i%8 <= 1) {
			attacks_knight[i] &= ~file[6];
			attacks_knight[i] &= ~file[7];
		} else if (i%8 >= 6) {
			attacks_knight[i] &= ~file[0];
			attacks_knight[i] &= ~file[1];
		}
	}
}