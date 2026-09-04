#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include "RenderInfo.h"
#include "ResourceManager.h"

using namespace Microsoft::WRL;

class ResourceManager;

class Renderer
{
public:
	bool Initialize(
		ID3D11Device* device,
		ID3D11DeviceContext* context,
		ResourceManager* resources,
		Vector2 viewportSize
	);

	void Draw(const RenderInfo& info);
	void Begin();

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
		bool flipX = false
	);
	bool CreateGeometry();
	bool CreateShaders();
	bool CreateBlendState();
	bool CreateSamplerState();

private:
	// Application이 ownership을 가진 ComPtr 객체 빌려쓰기
	ID3D11Device* device_ = nullptr;
	ID3D11DeviceContext* context_ = nullptr;

	ResourceManager* resources_ = nullptr;

	Vector2 viewportSize_{ 1.0f, 1.0f };

	//Buffer
	ComPtr<ID3D11Buffer> vertexBuffer_;
	ComPtr<ID3D11Buffer> indexBuffer_;

	//Shader
	ComPtr<ID3D11VertexShader> vertexShader_;
	ComPtr<ID3D11PixelShader> pixelShader_;
	ComPtr<ID3D11InputLayout> inputLayout_;

	//Blend State
	ComPtr<ID3D11BlendState> blendState_;

	//Sampler State
	ComPtr<ID3D11SamplerState> samplerState_;
};
