#include "Renderer.h"

// HLSL 컴파일러
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

// 정점 structure
struct Vertex
{
    float x, y, z;
    float u, v;
    float r, g, b, a;
};

bool Renderer::Initialize(
	ID3D11Device* device,
	ID3D11DeviceContext* context,
	ResourceManager* resources,
	Profiler* profiler,
	Vector2 viewportSize)
{
    device_ = device;
    context_ = context;
	resources_ = resources;
	profiler_ = profiler;

    viewportSize_ = viewportSize;

	if (!CreateGeometry()) return false;
	if (!CreateWhiteTexture()) return false;
	if (!CreateShaders()) return false;
	if (!CreateSamplerState()) return false;
	if (!CreateBlendState()) return false;

    return true;
}

void Renderer::Draw(
	const RenderInfo& info)
{
	if (!info.visible)
		return;

	if (info.spriteId ==SpriteId::None)return;

	auto* texture =resources_->GetTexture(info.spriteId);

	if (texture == nullptr)
		return;

	const float frameWidthUV = 1.0f /
		static_cast<float>(info.frameCount);

	const float u0 =
		static_cast<float>(info.frame) * frameWidthUV;

	const float u1 =
		u0 + frameWidthUV;

	const float spriteAspect =
		info.frameSizePixels.x / info.frameSizePixels.y;

	Vector2 halfSize = info.renderHalfSize;
	if (halfSize.x <= 0.0f)
	{
		halfSize.x = halfSize.y * spriteAspect * viewportSize_.y / viewportSize_.x;
	}

	DrawSprite(
		texture,
		info.position + info.offset,
		halfSize,
		{ u0, 0.0f },
		{ u1, 1.0f },
		info.flipX
	);
}

void Renderer::DrawSprite(
	ID3D11ShaderResourceView* textureView,
	Vector2 position,
	Vector2 halfSize,
	Vector2 uvMin,
	Vector2 uvMax,
	bool flipX,
	RendererColor color,
	bool countForProfiler)
{
	float leftU = flipX ? uvMax.x : uvMin.x;
	float rightU = flipX ? uvMin.x : uvMax.x;

	const Vertex vertices[] =
	{
		{ position.x - halfSize.x, position.y - halfSize.y, 0.0f, leftU, uvMax.y, color.r, color.g, color.b, color.a },
		{ position.x + halfSize.x, position.y - halfSize.y, 0.0f, rightU, uvMax.y, color.r, color.g, color.b, color.a },
		{ position.x + halfSize.x, position.y + halfSize.y, 0.0f, rightU, uvMin.y, color.r, color.g, color.b, color.a },
		{ position.x - halfSize.x, position.y + halfSize.y, 0.0f, leftU, uvMin.y, color.r, color.g, color.b, color.a }
	};

	// vertex buffer를 CPU가 접근 가능한 메모리 영역으로 매핑
	D3D11_MAPPED_SUBRESOURCE mappedResource{};

	HRESULT hr = context_->Map(
		vertexBuffer_.Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mappedResource
	);

	if (FAILED(hr)) return;

	// 새 vertex 데이터 넣기
	memcpy(mappedResource.pData, vertices, sizeof(vertices));

	// CPU 작업 끝, GPU가 Resource 사용
	context_->Unmap(vertexBuffer_.Get(), 0);

	// 이번 Draw에서 사용할 Texture
	context_->PSSetShaderResources(0, 1, &textureView);
	context_->DrawIndexed(6, 0, 0);
	// 실제 Draw 명령을 제출한 뒤 세어 실패하거나 생략된 Sprite는 제외한다.
	if (countForProfiler && profiler_ != nullptr)
	{
		// 현 Renderer에서는 스프라이트 한 장마다 Draw Call 하나가 발생한다.
		profiler_->Increment(ProfileCounter::SpriteDraws);
		profiler_->Increment(ProfileCounter::DrawCalls);
	}
}

void Renderer::DrawUIRect(Vector2 position, Vector2 halfSize, RendererColor color)
{
	// 흰색 1x1 텍스처에 정점 색상을 곱해 UI 배경 사각형을 그린다.
	DrawSprite(whiteTextureView_.Get(), position, halfSize, {}, { 1.0f, 1.0f }, false, color, false);
}

void Renderer::DrawUITexture(
	ID3D11ShaderResourceView* texture,
	Vector2 position,
	Vector2 halfSize)
{
	// Profiler 패널 자체의 Draw Call은 측정 카운터에 넣지 않는다.
	if (texture == nullptr) return;
	DrawSprite(texture, position, halfSize, {}, { 1.0f, 1.0f }, false, {}, false);
}

void Renderer::DrawSprite(
	SpriteId id,
	Vector2 position,
	Vector2 halfSize)
{
	auto* texture =
		resources_->GetTexture(id);

	if (texture == nullptr)
		return;

	DrawSprite(
		texture,
		position,
		halfSize,
		{ 0.0f, 0.0f },
		{ 1.0f, 1.0f },
		false
	);
}

bool Renderer::CreateGeometry()
{
	float u0 = 0.0f;
	float u1 = 0.25f;

	// 사각형 vertex 데이터
	Vertex vertices[] =
	{
		{ -0.5f, -0.5f, 0.0f, u0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
		{ 0.5f, -0.5f, 0.0f, u1, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
		{ 0.5f,  0.5f, 0.0f, u1, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f },
		{ -0.5f,  0.5f, 0.0f, u0, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f }
	};

	// vertex 버퍼 생성
	D3D11_BUFFER_DESC bufferDesc{};
	bufferDesc.ByteWidth = sizeof(vertices);

	// vertex 퍼버 Default -> Dynamic으로 변경
	// Default : 일반적인 정적 Geometry에 적합
	// Dynamic : CPU가 자주 내용을 갱신하는 Resource
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	// 매 애니메이션 프레임마다 UV 변경 -> vertex buffer 수정

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = vertices;

	// Device에 vertex buffer 리소스 생성 요청
	HRESULT hr = device_->CreateBuffer(&bufferDesc, &initData, vertexBuffer_.GetAddressOf());

	if (FAILED(hr)) return false;

	// 사각형 index 데이터, 삼각형 2개로 구성
	unsigned int indices[] = { 0, 2, 1, 0, 3, 2 }; // index를 사용하여 중복되는 vertex를 재사용.

	bufferDesc.ByteWidth = sizeof(indices);
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	initData.pSysMem = indices;

	hr = device_->CreateBuffer(&bufferDesc, &initData, indexBuffer_.GetAddressOf());

	return SUCCEEDED(hr);
}

bool Renderer::CreateWhiteTexture()
{
	// UI 단색 사각형을 기존 텍스처 셰이더 경로로 그릴 1픽셀 흰색 텍스처다.
	const unsigned int whitePixel = 0xFFFFFFFF;
	D3D11_TEXTURE2D_DESC textureDesc{};
	textureDesc.Width = 1;
	textureDesc.Height = 1;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA initialData{};
	initialData.pSysMem = &whitePixel;
	initialData.SysMemPitch = sizeof(whitePixel);

	ComPtr<ID3D11Texture2D> texture;
	HRESULT hr = device_->CreateTexture2D(
		&textureDesc, &initialData, texture.GetAddressOf());
	if (FAILED(hr)) return false;

	return SUCCEEDED(device_->CreateShaderResourceView(
		texture.Get(), nullptr, whiteTextureView_.GetAddressOf()));
}

bool Renderer::CreateShaders()
{
	ComPtr<ID3DBlob> vertexShaderBlob; // 컴파일 결과는 ID3DBlob라는 바이너리 덩어리로 나옴
	ComPtr<ID3DBlob> pixelShaderBlob; // hlsl -> compile -> Shader Bytecode -> ID3DBlob

	// Vertex Shader 컴파일
	HRESULT hr = D3DCompileFromFile(
		L"Shaders/BasicVs.hlsl",
		nullptr,
		nullptr,
		"main", // entry point
		"vs_5_0", // model version
		0,
		0,
		vertexShaderBlob.GetAddressOf(),
		nullptr
	);

	if (FAILED(hr)) return false;

	// Pixel Shader 컴파일
	hr = D3DCompileFromFile(
		L"Shaders/BasicPS.hlsl",
		nullptr,
		nullptr,
		"main", // entry point
		"ps_5_0", // model version
		0,
		0,
		pixelShaderBlob.GetAddressOf(),
		nullptr
	);

	if (FAILED(hr)) return false;

	// Device에 VertexShader 생성 요청 (Context)
	hr = device_->CreateVertexShader(
		vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize(),
		nullptr,
		vertexShader_.GetAddressOf()
	);

	if (FAILED(hr)) return false;

	// Device에 Pixel Shader 생성 요청 (Context)
	hr = device_->CreatePixelShader(
		pixelShaderBlob->GetBufferPointer(),
		pixelShaderBlob->GetBufferSize(),
		nullptr,
		pixelShader_.GetAddressOf()
	);

	if (FAILED(hr)) return false;

	// Input Layout
	D3D11_INPUT_ELEMENT_DESC inputElements[] =
	{
		{"POSITION",0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	hr = device_->CreateInputLayout(
		inputElements,
		3,
		vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize(),
		inputLayout_.GetAddressOf()
	);

	return SUCCEEDED(hr);
}

bool Renderer::CreateBlendState()
{
	D3D11_BLEND_DESC blendDesc{};

	blendDesc.RenderTarget[0].BlendEnable = TRUE;

	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;

	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;

	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;

	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;

	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

	blendDesc.RenderTarget[0].RenderTargetWriteMask =
		D3D11_COLOR_WRITE_ENABLE_ALL;

	HRESULT hr = device_->CreateBlendState(&blendDesc, blendState_.GetAddressOf());

	return SUCCEEDED(hr);
}

bool Renderer::CreateSamplerState()
{
	D3D11_SAMPLER_DESC desc{};

	desc.Filter =
		D3D11_FILTER_MIN_MAG_MIP_POINT;

	desc.AddressU =
		D3D11_TEXTURE_ADDRESS_CLAMP;

	desc.AddressV =
		D3D11_TEXTURE_ADDRESS_CLAMP;

	desc.AddressW =
		D3D11_TEXTURE_ADDRESS_CLAMP;

	desc.MinLOD = 0;
	desc.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT hr =
		device_->CreateSamplerState(
			&desc,
			samplerState_.GetAddressOf()
		);

	return SUCCEEDED(hr);
}

void Renderer::Begin()
{
	// Input Assembler 설정

	UINT stride = sizeof(Vertex); // 다음 vertex 값을 읽기 위해서 이동하는 byte 수, 즉 sizeof(Vertex)
	UINT offset = 0;

	// Vertex Buffer 연결
	context_->IASetVertexBuffers(0, 1, vertexBuffer_.GetAddressOf(), &stride, &offset);

	// Index Buffer 연결
	context_->IASetIndexBuffer(indexBuffer_.Get(), DXGI_FORMAT_R32_UINT, 0);

	// Vertex 구조
	context_->IASetInputLayout(inputLayout_.Get());

	// Index 3개마다 하나의 Triangle로
	context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Shader 설정
	context_->VSSetShader(vertexShader_.Get(), nullptr, 0);
	context_->PSSetShader(pixelShader_.Get(), nullptr, 0);

	// Sampler State 가져오기
	ID3D11SamplerState* sampler = samplerState_.Get();

	// Pixel Shader Texture slot 0에 Sampler 연결
	context_->PSSetSamplers(0, 1, samplerState_.GetAddressOf());

	// Blend State 적용
	context_->OMSetBlendState(blendState_.Get(), nullptr, 0xFFFFFFFF);
}
