#include "Core/Engine.h"
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
		std::cout << "fatal exception in main: " << e.what() << '\n';
		return 1; 
	}
	return 0;
}