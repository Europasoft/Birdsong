#include "core/engine/Engine.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

// execution entry point
int main()
{
	// create application object
	EngineCore::EngineApplication engine {};

	try
	{
		engine.startExecution();
	}
	catch (const std::exception& e) 
	{ 
		std::cout << "\033[31m" << " Fatal exception in main: " << e.what() << "\033[0m" << '\n';
		return 1; 
	}
	return 0;
}
