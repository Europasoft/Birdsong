
// name of the factory function that will be defined in DLL code
#define IG_FACTORY CreateIGameInstance

// macro used in DLL code to define the factory function which creates an instance of the game class
#ifdef _WIN32
	#define GAME_MAIN_FACTORY(GameClass) extern "C" __declspec(dllexport) EngineInterface::IGame* IG_FACTORY() { return new GameClass(); }
#endif
// Linux version
#ifndef _WIN32
	#define GAME_MAIN_FACTORY(GameClass) extern "C" __attribute__((visibility("default"))) EngineInterface::IGame* IG_FACTORY() { return new GameClass(); }
#endif

// factory function name as a string, to make absolutely sure we both declare and get the function with the exact same name
#define IG_FAC_STRINGIFY_HELPER(x) #x
#define IG_FAC_STRINGIFY(x) IG_FAC_STRINGIFY_HELPER(x)
static constexpr auto igameFactoryNameString = IG_FAC_STRINGIFY(IG_FACTORY);