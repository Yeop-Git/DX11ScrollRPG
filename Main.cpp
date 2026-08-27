#include <Windows.h>
#include "Application.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
	Application app;

	if (!app.Initialize(hInstance, nCmdShow))
	{
		return -1;
	}

	return app.Run();
}