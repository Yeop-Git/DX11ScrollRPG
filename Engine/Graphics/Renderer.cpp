#include "Renderer.h"
#include "UIRadialFill.h"

// HLSL 컴파일러
#include <d3dcompiler.h>

#include <cstdint>
#include <cstring>
#include <chrono>

#pragma comment(lib, "d3dcompiler.lib")

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
	if (!CreateDepthStencilStates()) return false;

    return true;
}

void Renderer::Draw(
	const RenderInfo& info)
{
	Draw(info, 0, false);
}

void Renderer::Draw(
	const RenderInfo& info,
	std::size_t renderLayer,
	bool reorderSafe)
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

	const Vector2 position = info.position + info.offset;
	if (collectQueue_
		&& reorderSafe
		&& renderQueueEnabled_
		&& info.renderMode == SpriteRenderMode::Cutout)
	{
		const QueueKey key{ renderLayer, texture, info.renderMode, true };
		auto bucket = queueLookup_.find(key);
		if (bucket == queueLookup_.end())
		{
			const std::size_t bucketIndex = activeQueueBucketCount_++;
			if (bucketIndex == queueBuckets_.size())
			{
				queueBuckets_.emplace_back();
			}

			QueueBucket& newBucket = queueBuckets_[bucketIndex];
			newBucket.key = key;
			newBucket.sprites.clear();
			bucket = queueLookup_.emplace(key, bucketIndex).first;
		}

		QueuedSprite queued{};
		queued.texture = texture;
		queued.position = position;
		queued.halfSize = halfSize;
		queued.uvMin = { u0, 0.0f };
		queued.uvMax = { u1, 1.0f };
		queued.flipX = info.flipX;
		queued.depth = info.depth;
		queued.renderMode = info.renderMode;
		queueBuckets_[bucket->second].sprites.push_back(queued);
		return;
	}

	DrawSprite(
		texture,
		position,
		halfSize,
		{ u0, 0.0f },
		{ u1, 1.0f },
		info.flipX,
		{},
		true,
		info.depth,
		info.renderMode
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
	bool countForProfiler,
	float depth,
	SpriteRenderMode renderMode)
{
	float leftU = flipX ? uvMax.x : uvMin.x;
	float rightU = flipX ? uvMin.x : uvMax.x;

	const Vertex vertices[] =
	{
		{ position.x - halfSize.x, position.y - halfSize.y, depth, leftU, uvMax.y, color.r, color.g, color.b, color.a },
		{ position.x + halfSize.x, position.y - halfSize.y, depth, rightU, uvMax.y, color.r, color.g, color.b, color.a },
		{ position.x + halfSize.x, position.y + halfSize.y, depth, rightU, uvMin.y, color.r, color.g, color.b, color.a },
		{ position.x - halfSize.x, position.y + halfSize.y, depth, leftU, uvMin.y, color.r, color.g, color.b, color.a }
	};

	// 텍스처와 계측 정책이 달라지거나 용량이 차면 순서를 유지한 채 현재 묶음을 제출한다.
	if (batchSpriteCount_ > 0
		&& (batchTexture_ != textureView
			|| batchRenderMode_ != renderMode
			|| batchCountForProfiler_ != countForProfiler
			|| batchSpriteCount_ >= kMaxSpritesPerBatch))
	{
		Flush();
	}

	if (batchSpriteCount_ == 0)
	{
		batchTexture_ = textureView;
		batchRenderMode_ = renderMode;
		batchCountForProfiler_ = countForProfiler;
	}

	batchVertices_.insert(batchVertices_.end(), vertices, vertices + 4);
	++batchSpriteCount_;
	if (!batchingEnabled_)
	{
		Flush();
	}
}

void Renderer::FlushRenderQueue()
{
	if (activeQueueBucketCount_ == 0)
	{
		return;
	}

	// Bucket는 최초 등장 순서로 drain한다. 레이어 입력 순서와 버킷 내부 순서를 보존한다.
	for (std::size_t bucketIndex = 0;
		bucketIndex < activeQueueBucketCount_;
		++bucketIndex)
	{
		const QueueBucket& bucket = queueBuckets_[bucketIndex];
		for (const QueuedSprite& sprite : bucket.sprites)
		{
			DrawSprite(
				sprite.texture,
				sprite.position,
				sprite.halfSize,
				sprite.uvMin,
				sprite.uvMax,
				sprite.flipX,
				sprite.color,
				sprite.countForProfiler,
				sprite.depth,
				sprite.renderMode);
		}
	}
	Flush();
	if (profiler_ != nullptr)
	{
		profiler_->AddTime(
			ProfileCategory::RenderQueue,
			std::chrono::duration<double, std::milli>(
				std::chrono::steady_clock::now() - queueStartTime_).count());
	}
	ClearRenderQueue();
	if (collectQueue_)
	{
		queueStartTime_ = std::chrono::steady_clock::now();
	}
}

void Renderer::ClearRenderQueue()
{
	queueLookup_.clear();
	for (QueueBucket& bucket : queueBuckets_)
	{
		bucket.sprites.clear();
	}
	activeQueueBucketCount_ = 0;
}

void Renderer::Flush()
{
	if (batchSpriteCount_ == 0)
	{
		return;
	}

	D3D11_MAPPED_SUBRESOURCE mappedResource{};
	const HRESULT hr = context_->Map(
		vertexBuffer_.Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mappedResource);

	if (FAILED(hr))
	{
		// 실패한 묶음은 재사용하지 않고 버려 뒤의 다른 상태 Sprite와 섞이지 않게 한다.
		ClearBatch();
		return;
	}

	const std::size_t vertexBytes = batchVertices_.size() * sizeof(Vertex);
	std::memcpy(mappedResource.pData, batchVertices_.data(), vertexBytes);
	context_->Unmap(vertexBuffer_.Get(), 0);

	context_->PSSetShaderResources(0, 1, &batchTexture_);
	context_->PSSetShader(
		batchRenderMode_ == SpriteRenderMode::Cutout
			? cutoutPixelShader_.Get()
			: pixelShader_.Get(),
		nullptr,
		0);
	context_->OMSetBlendState(
		batchRenderMode_ == SpriteRenderMode::Cutout
			? opaqueBlendState_.Get()
			: blendState_.Get(),
		nullptr,
		0xFFFFFFFF);
	ID3D11DepthStencilState* depthState = depthDisabledState_.Get();
	if (useDepthBuffer_)
	{
		depthState = batchRenderMode_ == SpriteRenderMode::Cutout
			? depthWriteState_.Get()
			: depthReadOnlyState_.Get();
	}
	context_->OMSetDepthStencilState(depthState, 0);
	context_->DrawIndexed(static_cast<UINT>(batchSpriteCount_ * 6), 0, 0);

	// 카운터는 실제 제출 단위로 기록한다. Profiler 전용 UI Batch는 계속 제외한다.
	if (batchCountForProfiler_ && profiler_ != nullptr)
	{
		profiler_->Increment(
			ProfileCounter::SpriteDraws,
			static_cast<std::uint64_t>(batchSpriteCount_));
		profiler_->Increment(ProfileCounter::DrawCalls);
	}

	ClearBatch();
}

void Renderer::ClearBatch()
{
	batchVertices_.clear();
	batchTexture_ = nullptr;
	batchSpriteCount_ = 0;
	batchCountForProfiler_ = true;
	batchRenderMode_ = SpriteRenderMode::Cutout;
}

void Renderer::DrawUIRect(Vector2 position, Vector2 halfSize, RendererColor color)
{
	// 흰색 1x1 텍스처에 정점 색상을 곱해 UI 배경 사각형을 그린다.
	DrawSprite(whiteTextureView_.Get(), position, halfSize, {}, { 1.0f, 1.0f }, false, color, false, 0.0f, SpriteRenderMode::AlphaBlend);
}

void Renderer::DrawUITexture(
	ID3D11ShaderResourceView* texture,
	Vector2 position,
	Vector2 halfSize,
	Vector2 uvMin,
	Vector2 uvMax)
{
	// Profiler 패널 자체의 Draw Call은 측정 카운터에 넣지 않는다.
	if (texture == nullptr) return;
	DrawSprite(texture, position, halfSize, uvMin, uvMax, false, {}, false, 0.0f, SpriteRenderMode::AlphaBlend);
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
		false,
		{},
		true,
		0.5f,
		SpriteRenderMode::Cutout
	);
}

bool Renderer::CreateGeometry()
{
	batchVertices_.reserve(kMaxSpritesPerBatch * 4);
	queueBuckets_.reserve(16);
	queueLookup_.reserve(16);

	// CPU가 프레임마다 갱신하는 정점 버퍼는 최대 2,048 사각형을 수용한다.
	D3D11_BUFFER_DESC vertexBufferDesc{};
	vertexBufferDesc.ByteWidth = static_cast<UINT>(
		sizeof(Vertex) * kMaxSpritesPerBatch * 4);
	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr = device_->CreateBuffer(
		&vertexBufferDesc,
		nullptr,
		vertexBuffer_.GetAddressOf());
	if (FAILED(hr)) return false;

	// 사각형마다 기존 winding을 유지하는 6개 인덱스를 만들어 모든 Batch에서 재사용한다.
	std::vector<unsigned int> indices(kMaxSpritesPerBatch * 6);
	for (std::size_t spriteIndex = 0; spriteIndex < kMaxSpritesPerBatch; ++spriteIndex)
	{
		const unsigned int vertexBase = static_cast<unsigned int>(spriteIndex * 4);
		const std::size_t indexBase = spriteIndex * 6;
		indices[indexBase] = vertexBase;
		indices[indexBase + 1] = vertexBase + 2;
		indices[indexBase + 2] = vertexBase + 1;
		indices[indexBase + 3] = vertexBase;
		indices[indexBase + 4] = vertexBase + 3;
		indices[indexBase + 5] = vertexBase + 2;
	}

	D3D11_BUFFER_DESC indexBufferDesc{};
	indexBufferDesc.ByteWidth = static_cast<UINT>(indices.size() * sizeof(unsigned int));
	indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA indexData{};
	indexData.pSysMem = indices.data();
	hr = device_->CreateBuffer(
		&indexBufferDesc,
		&indexData,
		indexBuffer_.GetAddressOf());

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
	ComPtr<ID3DBlob> cutoutPixelShaderBlob;

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

	hr = D3DCompileFromFile(
		L"Shaders/BasicPS.hlsl",
		nullptr,
		nullptr,
		"mainCutout",
		"ps_5_0",
		0,
		0,
		cutoutPixelShaderBlob.GetAddressOf(),
		nullptr);
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

	hr = device_->CreatePixelShader(
		cutoutPixelShaderBlob->GetBufferPointer(),
		cutoutPixelShaderBlob->GetBufferSize(),
		nullptr,
		cutoutPixelShader_.GetAddressOf());
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
	if (FAILED(hr)) return false;

	blendDesc.RenderTarget[0].BlendEnable = FALSE;
	return SUCCEEDED(device_->CreateBlendState(
		&blendDesc, opaqueBlendState_.GetAddressOf()));
}

bool Renderer::CreateDepthStencilStates()
{
	D3D11_DEPTH_STENCIL_DESC desc{};
	desc.DepthEnable = TRUE;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	desc.DepthFunc = D3D11_COMPARISON_LESS;
	HRESULT hr = device_->CreateDepthStencilState(
		&desc, depthWriteState_.GetAddressOf());
	if (FAILED(hr)) return false;

	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	hr = device_->CreateDepthStencilState(
		&desc, depthReadOnlyState_.GetAddressOf());
	if (FAILED(hr)) return false;

	desc.DepthEnable = FALSE;
	return SUCCEEDED(device_->CreateDepthStencilState(
		&desc, depthDisabledState_.GetAddressOf()));
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
	Begin(depthBufferEnabled_, renderQueueEnabled_);
}

void Renderer::Begin(bool useDepthBuffer)
{
	Begin(useDepthBuffer, false);
}

void Renderer::Begin(bool useDepthBuffer, bool collectQueue)
{
	useDepthBuffer_ = useDepthBuffer;
	collectQueue_ = collectQueue && renderQueueEnabled_;
	if (collectQueue_)
	{
		ClearRenderQueue();
		queueStartTime_ = std::chrono::steady_clock::now();
	}
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

}

void Renderer::DrawUIIcon(SpriteId icon, Vector2 position, Vector2 halfSize)
{
	DrawUITexture(resources_->GetTexture(icon), position, halfSize);
}

void Renderer::DrawUITriangle(Vector2 a, Vector2 b, Vector2 c, RendererColor color)
{
	// 기존 사각형 인덱스를 재사용. 첫 삼각형만 출력하고 두 번째는 퇴화시킨다.
	if (batchSpriteCount_ > 0 &&
		(batchTexture_ != whiteTextureView_.Get() || batchRenderMode_ != SpriteRenderMode::AlphaBlend ||
		 batchCountForProfiler_ || batchSpriteCount_ >= kMaxSpritesPerBatch))
		Flush();
	if (batchSpriteCount_ == 0)
	{
		batchTexture_ = whiteTextureView_.Get();
		batchRenderMode_ = SpriteRenderMode::AlphaBlend;
		batchCountForProfiler_ = false;
	}
	for (const auto point : {a, c, b, a})
		batchVertices_.push_back({point.x, point.y, 0.0f, 0.0f, 0.0f, color.r, color.g, color.b, color.a});
	++batchSpriteCount_;
	if (!batchingEnabled_)
		Flush();
}

void Renderer::DrawUICooldown(Vector2 position, Vector2 halfSize, float remainingFraction,
							  RendererColor color)
{
	// 정규화된 화면 좌표를 Clip 좌표로 변환하며 Y 방향만 반전한다.
	const auto toClip = [&](Vector2 point) {
		return Vector2{position.x + point.x * halfSize.x, position.y - point.y * halfSize.y};
	};
	for (const auto& triangle : UIRadialFill::Remaining(remainingFraction))
		DrawUITriangle(toClip(triangle[0]), toClip(triangle[1]), toClip(triangle[2]), color);
}
