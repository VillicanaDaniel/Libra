#include "Game/App.hpp"

#define WIN32_LEAN_AND_MEAN	
#include <windows.h>

#define UNUSED(x) (void)(x);

//-----------------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE applicationInstanceHandle, HINSTANCE, LPSTR commandLineString, int)
{
	UNUSED(applicationInstanceHandle);
	UNUSED(commandLineString);

	g_theApp = new App();
	g_theApp->RunMainLoop();
	delete g_theApp;
	g_theApp = nullptr;

	return 0;
}