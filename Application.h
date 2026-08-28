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
	bool CreateGeometry();

	bool ProcessMessages();
	void Update();
	void Render();

private:
	// Windows 창 Handle
	HWND hwnd_ = nullptr;

	// constexpr : 컴파일 시점에 값이 결정되는 상수
	static constexpr int kWindowWidth = 1280;
	static constexpr int kWindowHeight = 720;

	// DirectX 핵심 객체
	// ComPtr : 스마트 포인터, 참조 카운트 기반 COM 객체 관리(RAII)
	Microsoft::WRL::ComPtr<ID3D11Device> device_;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView_;

	// GPU에 저장되는 geometry 데이터
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer_;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer_;
};