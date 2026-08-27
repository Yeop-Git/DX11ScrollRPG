#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>

class Application
{
public:
	Application() = default;
	~Application() = default;

	bool Initialize(HINSTANCE hInstance, int nCmdShow);
	int Run();

private:
	bool CreateMainWindow(HINSTANCE hInstance, int nCmdShow);
	bool InitializeDirectX();

	bool ProcessMessages();
	void Update();
	void Render();

private:
	HWND hwnd_ = nullptr;

	static constexpr int kWInodwWidth = 1280;
	static constexpr int kWindowHeight = 720;

	Microsoft::WRL::ComPtr<ID3D11Device> device_;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView_;
};