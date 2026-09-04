#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <chrono>

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

	bool CreatePlayerTextures();
	bool CreateWorldTextures();
	bool CreateMonsterTextures();

	bool ProcessMessages();

	void Update(float deltaTime);
	void Render();
	float GetDeltaTime();

private:
	// Windows 창 Handle
	HWND hwnd_ = nullptr;

	// Update용 previousTime
	std::chrono::steady_clock::time_point previousTime_;

	// Components
	GameWorld gameWorld_;
	ResourceManager resourceManager_;
	Renderer renderer_;

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

	// Player Animation Texture
	ComPtr<ID3D11ShaderResourceView> idleTextureView_;
	ComPtr<ID3D11ShaderResourceView> runTextureView_;
	ComPtr<ID3D11ShaderResourceView> jumpStartTextureView_;
	ComPtr<ID3D11ShaderResourceView> jumpEndTextureView_;
	ComPtr<ID3D11ShaderResourceView> attackTextureView_;
	ComPtr<ID3D11ShaderResourceView> deadTextureView_;

	// Monster Animation Texture
	ComPtr<ID3D11ShaderResourceView> monsterIdleTextureView_;
	ComPtr<ID3D11ShaderResourceView> monsterChaseTextureView_;
	ComPtr<ID3D11ShaderResourceView> monsterHitTextureView_;
	ComPtr<ID3D11ShaderResourceView> monsterDeadTextureView_;

	// World Coordinate Texture
	ComPtr<ID3D11ShaderResourceView> groundTextureView_;
	ComPtr<ID3D11ShaderResourceView> treeTextureView_;
	ComPtr<ID3D11ShaderResourceView> backgroundTextureView_;
};
