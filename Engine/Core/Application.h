#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <chrono>

#include "Profiler.h"
#include "../../Game/Player.h"
#include "../../Game/Monster.h"
#include "../../Game/World/GameWorld.h"
#include "../Graphics/ResourceManager.h"
#include "../Graphics/Renderer.h"

using namespace Microsoft::WRL;

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

	void Update(float deltaTime);
	void Render();
	void RenderUI();
	void UpdateProfilerWindowTitle(float deltaTime);
	float GetDeltaTime();

private:
	// Windows 창 Handle
	HWND hwnd_ = nullptr;

	// Update용 previousTime
	std::chrono::steady_clock::time_point previousTime_;

	// Components
	GameWorld gameWorld_;
	ResourceManager resourceManager_;
	// Renderer보다 먼저 선언해 Renderer의 비소유 참조보다 오래 살게 한다.
	Profiler profiler_;
	Renderer renderer_;
	// 창 제목을 매 프레임 갱신하지 않도록 표시 간격을 누적한다.
	float profilerTitleTimer_ = 0.0f;

	// constexpr : 컴파일 시점에 값이 결정되는 상수
	// Window
	static constexpr Vector2 kWindowSize{ 1280.0f, 720.0f };

	// DirectX 핵심 객체
	// ComPtr : 스마트 포인터, 참조 카운트 기반 COM 객체 관리(RAII)
	ComPtr<ID3D11Device> device_;
	ComPtr<ID3D11DeviceContext> context_;
	ComPtr<IDXGISwapChain> swapChain_;
	ComPtr<ID3D11RenderTargetView> renderTargetView_;

	// GPU에 저장되는 geometry 데이터
	ComPtr<ID3D11Buffer> vertexBuffer_;
	ComPtr<ID3D11Buffer> indexBuffer_;

	//Shaders
	ComPtr<ID3D11VertexShader> vertexShader_;
	ComPtr<ID3D11PixelShader> pixelShader_;
	ComPtr<ID3D11InputLayout> inputLayout_;

	// Texture
	ComPtr<ID3D11ShaderResourceView> textureView_;
	ComPtr<ID3D11SamplerState> samplerState_;
	
	// Alpha Blending
	ComPtr<ID3D11BlendState> blendState_;
};
