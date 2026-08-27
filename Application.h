#pragma once

#include <Windows.h>

class Application
{
public:
	Application() = default;
	~Application() = default;

	bool Initialize(HINSTANCE hInstance, int nCmdShow);
	int Run();

private:
	bool CreateMainWindow(HINSTANCE hInstance, int nCmdShow);

private:
	HWND hwnd_ = nullptr;
	bool running_ = true;

	static constexpr int kWInodwWidth = 1280;
	static constexpr int kWindowHeight = 720;
};