#include "boost/tokenizer.hpp"

#include "engine.hpp"
#include "evaluator.hpp"
#include "types.hpp"
#include "uci.hpp"

using std::string;

const std::vector<string> c_ValidCommands = {
	"uci", "isready", "ucinewgame", "position", "go", "stop", "debug", "quit",
	"move", "board", "moves", "eval"
};

UCIProtocol::UCIProtocol(std::unique_ptr<Engine> engine):engine_(std::move(engine))
{
}

void UCIProtocol::recvMessage(const string& message)
{
	//std::cout << "<< " << message << std::endl;
	
	// Split into tokens
	boost::char_separator<char> separator(" \t");
	tokenizer tokens(message, separator);
	auto token = tokens.begin();
	
	// Find first recognized command
	string command;
	do {
		if (token == tokens.end()) return;
		command = *token;
		++token;
	} while (std::find(c_ValidCommands.begin(), c_ValidCommands.end(), command) == c_ValidCommands.end());
	
	if (command == "uci")
	{
		// Identify engine and options
		sendMessage("id name " + engine_->name());
		sendMessage("id author " + engine_->author());
		for (auto option : engine_->options()) {
			sendMessage("option " + option);
		}
		sendMessage("uciok");
	}
	else if (command == "isready")
	{
		// Wait until the engine is ready for a new command
		sendMessage("readyok");
	}
	else if (command == "ucinewgame")
	{
		// Start a new game
	}
	else if (command == "position")
	{
		// Enter a position
		if (!engine_->board().setPosition(tokens, token)) {
			engine_->board().setInitialPosition();
			sendMessage("info string error: invalid position");
		}
	}
	else if (command == "go")
	{
		// Start thinking
		engine_->Search();
	}
	else if (command == "stop")
	{
		// Stop thinking
	}
	else if (command == "debug")
	{
		// Enable/Disable debug mode
		bool debug_value = (token != tokens.end() && *token == "on");
		engine_->setDebug(debug_value);
	}
	else if (command == "quit")
	{
		// Quit the engine
		running_ = false;
	}
	else if (command == "move")
	{
		// Execute a move
		if (token != tokens.end()) {
			move_t move = engine_->board().parseMove(*token);
			engine_->board().printMove(std::cout, move);
			engine_->board().doMove(move);
		}
	}
	else if (command == "moves")
	{
		std::vector<move_t> movelist;
		engine_->board().generateMoves(movelist);
		std::cout << movelist.size() << " moves:" << std::endl;
		for (move_t move : movelist) {
			engine_->board().printMove(std::cout, move);
		}
	}
	else if (command == "board")
	{
		// Print the board state
		engine_->board().printBoard(std::cout);
	}
	else if (command == "eval")
	{
		Evaluator eval;
		score_t score = eval.evaluatePosition(engine_->board());
		std::cout << "Score: " << score << " [in 1/1000ths of a pawn]" << std::endl;
	}
}

void UCIProtocol::sendMessage(const string& message)
{
	std::cout << message << std::endl;
}

void UCIProtocol::run()
{
	running_ = true;
	while (running_) {
		string message;
		if (getline(std::cin, message, '\n')) {
			recvMessage(message);
		} else {
			running_ = false;
		}
	}
}