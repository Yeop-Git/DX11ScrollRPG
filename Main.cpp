#include <Windows.h>
#include "Engine/Core/Application.h"

//entry point
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
	// 프로그램 전체 담당 Application 객체
	Application app;

	// Win, DiretX 초기화
	if (!app.Initialize(hInstance, nCmdShow))
	{
		return -1;
	}

	// Game Loop 시작
	return app.Run();
}