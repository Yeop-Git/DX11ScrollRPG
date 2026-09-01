#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <chrono>

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

	bool CreateGeometry();
	bool CreateShaders();
	bool CreateTexture();
	bool CreateBlendState();

	bool ProcessMessages();

	void Update(float deltaTime);
	void Render();
	void UpdateSpriteUV();
	float GetDeltaTime();

private:
	// Windows 창 Handle
	HWND hwnd_ = nullptr;

	// Animation
	int currentFrame_ = 0;
	float animationTimer_ = 0.0f;
	std::chrono::steady_clock::time_point previousTime_;

	// Player Stat
	float playerX_ = 0.0f;
	float playerY_ = -0.2f;

	float velocityX_ = 0.0f;
	float velocityY_ = 0.0f;

	bool isGrounded_ = true;

	// constexpr : 컴파일 시점에 값이 결정되는 상수
	// Window
	static constexpr int kWindowWidth = 1280;
	static constexpr int kWindowHeight = 720;

	// Animation
	static constexpr int kIdleFrameCount = 4;

	// Player Stat
	static constexpr float kMoveSpeed = 0.8f;
	static constexpr float kJumpSpeed = 1.5f;
	static constexpr float kGravity = -3.0f;

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