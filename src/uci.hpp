#pragma once

#include <iostream>
#include <memory>

// Forward declarations
class Engine;

class UCIProtocol
{
public:
	UCIProtocol(std::unique_ptr<Engine> engine);
	void run();
	
	static void sendMessage(const std::string& message);
	
private:
	void recvMessage(const std::string& message);
	
	bool running_;
	std::unique_ptr<Engine> engine_;
};