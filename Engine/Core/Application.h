#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <chrono>

#include "../../Game/Player.h"
#include "../../Game/Monster.h"

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
	bool CreatePlayerTextures();
	bool CreateWorldTextures();
	bool CreateMonsterTextures();
	bool CreateBlendState();
	//bool CreateTexture();
	bool LoadTexture(const char* filePath, ComPtr<ID3D11ShaderResourceView>& outTextureView); // CreateTexture()를 대체

	bool ProcessMessages();

	void Update(float deltaTime);
	void Render();
	void UpdatePlayerAnimation(float deltaTime);
	void UpdateMonsterAnimation(float deltaTime);
	bool IsMonsterAnimationFinished() const;
	//void UpdateSpriteUV();
	float GetDeltaTime();

	// Draw Functions
	void DrawBackground();
	void DrawTrees();
	void DrawGround();
	void DrawPlayer();
	void DrawMonster();
	void DrawSprite(
		ID3D11ShaderResourceView* textureView,
		float x,
		float y,
		float halfWidth,
		float halfHeight,
		float u0 = 0.0f,
		float v0 = 0.0f,
		float u1 = 1.0f,
		float v1 = 1.0f,
		bool flipX = false
	);

	int GetCurrentFrameCount() const;
	int GetCurrentMonsterFrameCount() const;
	ID3D11ShaderResourceView* GetCurrentPlayerTexture() const;
	ID3D11ShaderResourceView* GetCurrentMonsterTexture() const;

private:
	// Windows 창 Handle
	HWND hwnd_ = nullptr;

	//Player
	Player player_;
	PlayerState previousPlayerState_ = PlayerState::Idle;

	Monster monster_;
	MonsterState previousMonsterState_ = MonsterState::Idle;
	int monsterCurrentFrame_ = 0;
	float monsterAnimationTimer_ = 0.0f;

	// Animation
	int currentFrame_ = 0;
	float animationTimer_ = 0.0f;
	std::chrono::steady_clock::time_point previousTime_;

	// constexpr : 컴파일 시점에 값이 결정되는 상수
	// Window
	static constexpr int kWindowWidth = 1280;
	static constexpr int kWindowHeight = 720;

	// Player Animation Frame Count
	static constexpr int kIdleFrameCount = 4;
	static constexpr int kRunFrameCount = 8;
	static constexpr int kJumpStartFrameCount = 4;
	static constexpr int kJumpEndFrameCount = 3;

	// Enemy Animation Frame Count
	static constexpr int kMonsterIdleFrameCount = 4;
	static constexpr int kMonsterChaseFrameCount = 8;
	static constexpr int kMonsterHitFrameCount = 8;
	static constexpr int kMonsterDeadFrameCount = 8;

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