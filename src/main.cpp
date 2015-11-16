#include <iostream>
#include <memory>

#include "Crafty/MagicMoves.hpp"
#include "data.hpp"
#include "engine.hpp"
#include "random.hpp"
#include "uci.hpp"

int main()
{
	Random::AutoSeed();
	Data::initialize();
	initmagicmoves();
	
	UCIProtocol uci(std::unique_ptr<Engine>(new Engine()));
	uci.run();
	return 0;
}