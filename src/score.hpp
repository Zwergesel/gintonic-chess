#pragma once

namespace Score
{
	
	// Special scores
	const score_t command_stop = 23000;
	const score_t unknown = 22000;
	const score_t infinity = 21000;
	const score_t checkmate = 20000;
	const score_t mate_bound = 19500;
	const score_t stalemate = 0;
	
	// White pieces: ___, pawn, knight, king, ___, bishop, rook, queen
	// Black pieces: ___, pawn, knight, king, ___, bishop, rook, queen
	const score_t pieces[] = {
		0,  1000,  3200, 0, 0,  3300,  5000,  9000,
		0, -1000, -3200, 0, 0, -3300, -5000, -9000
	};
	
	const score_t pawn_squares[] = {
		  0,   0,    0,    0,    0,    0,   0,   0,
		500, 500,  500,  500,  500,  500, 500, 500,
		100, 100,  200,  300,  300,  200, 100, 100,
		 50,  50,  100,  250,  250,  100,  50,  50,
		  0,   0,    0,  200,  200,    0,   0,   0,
		 50, -50, -100,    0,    0, -100, -50,  50,
		 50, 100,  100, -200, -200,  100, 100,  50,
		  0,   0,    0,    0,    0,    0,   0,   0
	};
	
	const score_t knight_squares[] = {
		-500, -400, -300, -300, -300, -300, -400, -500,
		-400, -200,    0,    0,    0,    0, -200, -400,
		-300,    0,  100,  150,  150,  100,    0, -300,
		-300,   50,  150,  200,  200,  150,   50, -300,
		-300,    0,  150,  200,  200,  150,    0, -300,
		-300,   50,  100,  150,  150,  100,   50, -300,
		-400, -200,    0,   50,   50,    0, -200, -400,
		-500, -400, -300, -300, -300, -300, -400, -500,
	};
	
	const score_t bishop_squares[] = {
		-200, -100, -100, -100, -100, -100, -100, -200,
		-100,    0,    0,    0,    0,    0,    0, -100,
		-100,    0,   50,  100,  100,   50,    0, -100,
		-100,   50,   50,  100,  100,   50,   50, -100,
		-100,    0,  100,  100,  100,  100,    0, -100,
		-100,  100,  100,  100,  100,  100,  100, -100,
		-100,   50,    0,    0,    0,    0,   50, -100,
		-200, -100, -100, -100, -100, -100, -100, -200,
	};
	
	const score_t queen_squares[] = {
		-200, -100, -100, -50, -50, -100, -100, -200,
		-100,    0,    0,   0,   0,    0,    0, -100,
		-100,    0,   50,  50,  50,   50,    0, -100,
		 -50,    0,   50,  50,  50,   50,    0,  -50,
		   0,    0,   50,  50,  50,   50,    0,  -50,
		-100,   50,   50,  50,  50,   50,    0, -100,
		-100,    0,   50,   0,   0,    0,    0, -100,
		-200, -100, -100, -50, -50, -100, -100, -200
	};
	
	const score_t king_squares_midgame[] = {
		-300, -400, -400, -500, -500, -400, -400, -300,
		-300, -400, -400, -500, -500, -400, -400, -300,
		-300, -400, -400, -500, -500, -400, -400, -300,
		-300, -400, -400, -500, -500, -400, -400, -300,
		-200, -300, -300, -400, -400, -300, -300, -200,
		-100, -200, -200, -200, -200, -200, -200, -100,
		 200,  200,    0,    0,    0,    0,  200,  200,
		 200,  300,  100,    0,    0,  100,  300,  200
	};
	
	const score_t king_squares_endgame[] = {
		-500, -400, -300, -200, -200, -300, -400, -500,
		-300, -200, -100,    0,    0, -100, -200, -300,
		-300, -100,  200,  300,  300,  200, -100, -300,
		-300, -100,  300,  400,  400,  300, -100, -300,
		-300, -100,  300,  400,  400,  300, -100, -300,
		-300, -100,  200,  300,  300,  200, -100, -300,
		-300, -300,    0,    0,    0,    0, -300, -300,
		-500, -300, -300, -300, -300, -300, -300, -500
	};
}