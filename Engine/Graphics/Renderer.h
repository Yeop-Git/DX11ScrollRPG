#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include "../Core/Profiler.h"
#include "RenderInfo.h"
#include "ResourceManager.h"

using namespace Microsoft::WRL;

class ResourceManager;

// UI 사각형의 정점별 색상과 투명도를 Renderer에 전달한다.
struct RendererColor
{
	float r = 1.0f;
	float g = 1.0f;
	float b = 1.0f;
	float a = 1.0f;
};

class Renderer
{
public:
	bool Initialize(
		ID3D11Device* device,
		ID3D11DeviceContext* context,
		ResourceManager* resources,
		Profiler* profiler,
		Vector2 viewportSize
	);

	void Draw(const RenderInfo& info);
	void Begin();
	// UI 전용 그리기는 게임 Sprite/Draw Call 카운터에서 제외한다.
	void DrawUIRect(Vector2 position, Vector2 halfSize, RendererColor color);
	void DrawUITexture(ID3D11ShaderResourceView* texture, Vector2 position, Vector2 halfSize);

	void DrawSprite(
		SpriteId id,
		Vector2 position,
		Vector2 halfSize);

private:
	void DrawSprite(
		ID3D11ShaderResourceView* textureView,
		Vector2 position,
		Vector2 halfSize,
		Vector2 uvMin = { 0.0f, 0.0f },
		Vector2 uvMax = { 1.0f, 1.0f },
		bool flipX = false,
		RendererColor color = {},
		bool countForProfiler = true
	);
	bool CreateGeometry();
	bool CreateWhiteTexture();
	bool CreateShaders();
	bool CreateBlendState();
	bool CreateSamplerState();

private:
	// Application이 ownership을 가진 ComPtr 객체 빌려쓰기
	ID3D11Device* device_ = nullptr;
	ID3D11DeviceContext* context_ = nullptr;

	ResourceManager* resources_ = nullptr;
	// Application 소유 Profiler를 빌려 쓰며 Renderer는 수명을 관리하지 않는다.
	Profiler* profiler_ = nullptr;

	Vector2 viewportSize_{ 1.0f, 1.0f };

	//Buffer
	ComPtr<ID3D11Buffer> vertexBuffer_;
	ComPtr<ID3D11Buffer> indexBuffer_;
	ComPtr<ID3D11ShaderResourceView> whiteTextureView_;

	//Shader
	ComPtr<ID3D11VertexShader> vertexShader_;
	ComPtr<ID3D11PixelShader> pixelShader_;
	ComPtr<ID3D11InputLayout> inputLayout_;

	//Blend State
	ComPtr<ID3D11BlendState> blendState_;

	//Sampler State
	ComPtr<ID3D11SamplerState> samplerState_;
};
