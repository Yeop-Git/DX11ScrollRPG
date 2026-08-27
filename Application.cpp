#include "Application.h"

namespace
{
	LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}

		return DefWindowProc(hwnd, message, wParam, lParam);
	}
}

bool Application::CreateMainWindow(HINSTANCE hInstance, int nCmdShow)
{
	const wchar_t* className = L"DX11ScrollRPGWindowClass";

	WNDCLASS windowClass{};
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = className;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);

	if (!RegisterClass(&windowClass))
	{
		return false;
	}

	RECT windowRect{ 0, 0, kWInodwWidth, kWindowHeight };

	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	hwnd_ = CreateWindowEx(
		0,
		className,
		L"DX11ScrollRPG",
		WS_OVERLAPPEDWINDOW,

		CW_USEDEFAULT,
		CW_USEDEFAULT,

		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,

		nullptr,
		nullptr,
		hInstance,
		nullptr
	);

	if (!hwnd_) return false;

	ShowWindow(hwnd_, nCmdShow);

	return true;
}

bool Application::Initialize(HINSTANCE hInstance, int nCmdShow)
{
	return CreateMainWindow(hInstance, nCmdShow);
}

int Application::Run()
{
	MSG message{};

	while (ProcessMessages())
	{
		//Update();
		//Render();
	}

	return static_cast<int>(message.wParam);
}

bool Application::ProcessMessages()
{
	MSG message{};
	while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
	{
		if (message.message == WM_QUIT)
		{
			return false;
		}
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
	return true;
}