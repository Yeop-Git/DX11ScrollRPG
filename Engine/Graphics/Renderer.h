#pragma once

#include <cstddef>
#include <vector>

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
	void Begin(bool useDepthBuffer);
	void SetDepthBufferEnabled(bool enabled) { depthBufferEnabled_ = enabled; }
	bool IsDepthBufferEnabled() const { return depthBufferEnabled_; }
	// 현재 모인 Sprite 정점을 GPU에 제출하고 CPU 측 Batch 저장소를 재사용한다.
	void Flush();
	// UI 전용 그리기는 게임 Sprite/Draw Call 카운터에서 제외한다.
	void DrawUIRect(Vector2 position, Vector2 halfSize, RendererColor color);
	void DrawUITexture(ID3D11ShaderResourceView* texture, Vector2 position, Vector2 halfSize);

	void DrawSprite(
		SpriteId id,
		Vector2 position,
		Vector2 halfSize);

private:
	struct Vertex
	{
		float x, y, z;
		float u, v;
		float r, g, b, a;
	};

	static constexpr std::size_t kMaxSpritesPerBatch = 2048;

	void DrawSprite(
		ID3D11ShaderResourceView* textureView,
		Vector2 position,
		Vector2 halfSize,
		Vector2 uvMin = { 0.0f, 0.0f },
		Vector2 uvMax = { 1.0f, 1.0f },
		bool flipX = false,
		RendererColor color = {},
		bool countForProfiler = true,
		float depth = 0.5f,
		SpriteRenderMode renderMode = SpriteRenderMode::Cutout
	);
	void ClearBatch();
	bool CreateGeometry();
	bool CreateWhiteTexture();
	bool CreateShaders();
	bool CreateBlendState();
	bool CreateDepthStencilStates();
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

	// 정점은 CPU에서 연속 메모리로 모은 뒤 Flush 시 Dynamic Vertex Buffer로 복사한다.
	std::vector<Vertex> batchVertices_;
	// ResourceManager 또는 Renderer 소유 텍스처를 빌려 참조하며 소유권은 갖지 않는다.
	ID3D11ShaderResourceView* batchTexture_ = nullptr;
	std::size_t batchSpriteCount_ = 0;
	bool batchCountForProfiler_ = true;

	//Shader
	ComPtr<ID3D11VertexShader> vertexShader_;
	ComPtr<ID3D11PixelShader> pixelShader_;
	ComPtr<ID3D11PixelShader> cutoutPixelShader_;
	ComPtr<ID3D11InputLayout> inputLayout_;

	// 알파 블렌딩과 불투명 컷아웃 렌더링은 별도 상태를 사용한다.
	ComPtr<ID3D11BlendState> blendState_;
	ComPtr<ID3D11BlendState> opaqueBlendState_;
	ComPtr<ID3D11DepthStencilState> depthWriteState_;
	ComPtr<ID3D11DepthStencilState> depthReadOnlyState_;
	ComPtr<ID3D11DepthStencilState> depthDisabledState_;
	bool depthBufferEnabled_ = true;
	bool useDepthBuffer_ = true;
	SpriteRenderMode batchRenderMode_ = SpriteRenderMode::Cutout;

	//Sampler State
	ComPtr<ID3D11SamplerState> samplerState_;
};
