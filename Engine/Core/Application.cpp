#include "Application.h"
#include <algorithm>

// HLSL 컴파일러
#include <d3dcompiler.h>

// Image 디코더
#include "../ThirdParty/stb/stb_image.h"


#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace std::chrono;

// 정점 structure
struct Vertex
{
	float x, y, z;
	float u, v;
	//float r, g, b, a;
};

// Windows -> Message Queue -> WindowProc
namespace
{
	LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_DESTROY:
			//창 닫기
			PostQuitMessage(0);
			return 0;
		}

		return DefWindowProc(hwnd, message, wParam, lParam);
	}
}


bool Application::Initialize(HINSTANCE hInstance, int nCmdShow)
{
	if (!CreateMainWindow(hInstance, nCmdShow)) return false;

	if (!InitializeDirectX()) return false;

	if (!CreateGeometry()) return false;

	if (!CreateShaders()) return false;

	if (!CreatePlayerTextures()) return false;

	if (!CreateMonsterTextures()) return false;

	if (!CreateWorldTextures()) return false;

	if (!CreateBlendState()) return false;

	previousTime_ = steady_clock::now();

	return true;
}

// Game Loop
int Application::Run()
{
	MSG message{};

	//반복
	while (ProcessMessages()) // 메시지 처리
	{
		//게임 상태 갱신
		Update(GetDeltaTime());
		//화면 그리기
		Render();
	}

	return static_cast<int>(message.wParam);
}


bool Application::CreateMainWindow(HINSTANCE hInstance, int nCmdShow)
{
	// Windows에 등록할 클래스 이름
	const wchar_t* className = L"DX11ScrollRPGWindowClass";

	// 초기화
	WNDCLASS windowClass{};

	// Windows가 메세지 전달할 함수 지정
	windowClass.lpfnWndProc = WindowProc;

	// 현재 실행 프로그램 인스턴스 핸들
	windowClass.hInstance = hInstance;

	// Window 클래스 이름
	windowClass.lpszClassName = className;

	// 기본 마우스 커서
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);

	// Windows에 해당 windowClass를 등록
	if (!RegisterClass(&windowClass))
	{
		return false;
	}

	// kWindowWidth * kWindowHight 창
	RECT windowRect{ 0, 0, kWindowWidth, kWindowHeight };

	// 실제 원하는 게임 화면 크기와 동일하게 windowRect를 조정 
	// (title bar, border 등을 고려)
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	// 실제 창 생성
	hwnd_ = CreateWindowEx(
		0,
		className,
		L"DX11ScrollRPG",
		WS_OVERLAPPEDWINDOW,

		CW_USEDEFAULT,
		CW_USEDEFAULT,

		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,

		nullptr,
		nullptr,
		hInstance,
		nullptr
	);

	if (!hwnd_) return false;

	ShowWindow(hwnd_, nCmdShow);

	return true;
}

bool Application::InitializeDirectX()
{
	// SwapChain 설정값
	DXGI_SWAP_CHAIN_DESC desc{};

	desc.BufferDesc.Width = kWindowWidth;
	desc.BufferDesc.Height = kWindowHeight;
	// Format : 색상 표현 방식
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	desc.SampleDesc.Count = 1;

	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	// 화면 버퍼 두개 사용 : 그리기 & 화면 출력
	desc.BufferCount = 2;

	desc.OutputWindow = hwnd_;
	desc.Windowed = TRUE;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	// Direct X 핵심 개체 생성
	HRESULT hr = D3D11CreateDeviceAndSwapChain( // Device, DeviceContext, SwapChain 한번에 생성하는 함수
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&desc,
		swapChain_.GetAddressOf(),
		device_.GetAddressOf(),
		nullptr,
		context_.GetAddressOf()
	); // Device : 리소스 생성, DeviceContext : GPU에 명령, SwapChain : 화면 버퍼 관리 (출력)

	// HRESULT : Windows API 성공/실패 여부 반환
	if (FAILED(hr)) return false;

	// BackBuffer 가져오기
	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer; // 화면 자체도 하나의 2D Texture인 것.

	hr = swapChain_->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
	if (FAILED(hr)) return false;

	// RenderTargetView 생성, 같은 리소스도 다른 용도로 사용 가능.
	hr = device_->CreateRenderTargetView(backBuffer.Get(), nullptr, renderTargetView_.GetAddressOf());
	if (FAILED(hr)) return false;

	// RenderTarget 연결
	context_->OMSetRenderTargets(1, renderTargetView_.GetAddressOf(), nullptr);

	// Viewport 생성
	D3D11_VIEWPORT viewport{};

	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	viewport.Width = static_cast<float>(kWindowWidth);
	viewport.Height = static_cast<float>(kWindowHeight);

	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	context_->RSSetViewports(1, &viewport);

	return true;
}

bool Application::CreateGeometry()
{
	float u0 = 0.0f;
	float u1 = 0.25f;

	// 사각형 vertex 데이터
	Vertex vertices[] =
	{
		{ -0.5f, -0.5f, 0.0f, u0, 1.0f },
		{ 0.5f, -0.5f, 0.0f, u1, 1.0f },
		{ 0.5f,  0.5f, 0.0f, u1, 0.0f },
		{ -0.5f,  0.5f, 0.0f, u0, 0.0f }
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

bool Application::CreateShaders()
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
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	hr = device_->CreateInputLayout(
		inputElements,
		2,
		vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize(),
		inputLayout_.GetAddressOf()
	);

	return SUCCEEDED(hr);
}

bool Application::CreatePlayerTextures()
{
	if (!LoadTexture("Assets/Textures/Player/PlayerIdle.png", idleTextureView_)) return false;
	if (!LoadTexture("Assets/Textures/Player/PlayerRun.png", runTextureView_)) return false;
	if (!LoadTexture("Assets/Textures/Player/PlayerJumpStart.png", jumpStartTextureView_)) return false;
	if (!LoadTexture("Assets/Textures/Player/PlayerJumpEnd.png", jumpEndTextureView_)) return false;
	return true;
}

bool Application::CreateMonsterTextures()
{
	if (!LoadTexture("Assets/Textures/Monster/MonsterIdle.png", monsterIdleTextureView_)) return false;
	if (!LoadTexture("Assets/Textures/Monster/MonsterChase.png", monsterChaseTextureView_)) return false;
	if (!LoadTexture("Assets/Textures/Monster/MonsterHit.png", monsterHitTextureView_)) return false;
	if (!LoadTexture("Assets/Textures/Monster/MonsterDead.png", monsterDeadTextureView_)) return false;
}

bool Application::CreateWorldTextures()
{
	if (!LoadTexture("Assets/Textures/World/Ground.png", groundTextureView_)) return false;
	if (!LoadTexture("Assets/Textures/World/Tree.png", treeTextureView_)) return false;
	if (!LoadTexture("Assets/Textures/World/Background.png", backgroundTextureView_)) return false;
	return true;
}
//bool Application::CreateTexture()
//{
//	int width = 0;
//	int height = 0;
//	int channels = 0;
//
//	// PNG를 메모리의 RGBA pixel 배열로 디코딩
//	unsigned char* pixels = stbi_load(
//		"Assets/Textures/Player.png",
//		&width,
//		&height,
//		&channels,
//		STBI_rgb_alpha
//	);
//
//	if (!pixels) return false;
//
//	// Texture 설정
//	D3D11_TEXTURE2D_DESC textureDesc{};
//
//	textureDesc.Width = static_cast<UINT>(width);
//	textureDesc.Height = static_cast<UINT>(height);
//
//	textureDesc.MipLevels = 1;
//	textureDesc.ArraySize = 1;
//
//	// stb_image에서 RGBA 8bit로
//	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//
//	// MSAA 사용 X
//	textureDesc.SampleDesc.Count = 1;
//
//	// DFAULT Resource
//	textureDesc.Usage = D3D11_USAGE_DEFAULT;
//
//	// Pixel Shader가 읽을 Texture, Shader Resource로 활용
//	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
//
//	// 초기 Texture 데이터
//	D3D11_SUBRESOURCE_DATA initialData{};
//
//	initialData.pSysMem = pixels;
//
//	// 한 줄의 byte 크기
//	// RGBA = pixel 하나당 4 bytes
//	initialData.SysMemPitch = static_cast<UINT>(width * 4);
//
//	// Texture Resource 생성
//	ComPtr<ID3D11Texture2D> texture;
//	HRESULT hr = device_->CreateTexture2D(&textureDesc, &initialData, texture.GetAddressOf());
//	stbi_image_free(pixels);
//	if (FAILED(hr)) return false;
//
//	// Texture를 Shader에서 읽을 수 있도록 View 생성
//	hr = device_->CreateShaderResourceView(texture.Get(), nullptr, textureView_.GetAddressOf());
//	if (FAILED(hr))return false;
//
//	// Sampling 방식 설정
//	D3D11_SAMPLER_DESC samplerDesc{};
//
//	// Point Sampling
//	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
//	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
//	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
//	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
//
//	hr = device_->CreateSamplerState(&samplerDesc, samplerState_.GetAddressOf());
//
//	return SUCCEEDED(hr);
//}

// file 경로를 받아 Shader Resource View를 만드는 범용 Texture Loader
// 상단의 CreateTexture()를 대체
bool Application::LoadTexture(const char* filePath, ComPtr<ID3D11ShaderResourceView>& outTextureView)
{
	int width = 0;
	int height = 0;
	int channels = 0;

	unsigned char* pixels = stbi_load(
		filePath,
		&width,
		&height,
		&channels,
		STBI_rgb_alpha
	);

	if (!pixels) return false;

	D3D11_TEXTURE2D_DESC textureDesc{};

	textureDesc.Width = static_cast<UINT>(width);
	textureDesc.Height = static_cast<UINT>(height);

	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;

	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	textureDesc.SampleDesc.Count = 1;

	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA initialData{};
	initialData.pSysMem = pixels;
	initialData.SysMemPitch = static_cast<UINT>(width * 4);

	ComPtr<ID3D11Texture2D> texture;

	HRESULT hr = device_->CreateTexture2D(
		&textureDesc,
		&initialData,
		texture.GetAddressOf()
	);

	stbi_image_free(pixels);

	if (FAILED(hr)) return false;

	hr = device_->CreateShaderResourceView(
		texture.Get(),
		nullptr,
		outTextureView.GetAddressOf()
	);

	return SUCCEEDED(hr);
}

bool Application::CreateBlendState()
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

bool Application::ProcessMessages()
{
	MSG message{};
	// 메세지 확인하고 바로 돌아오는 PeekMessage 사용 (GetMessage는 메세지가 올 때까지 대기)
	while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
	{
		if (message.message == WM_QUIT)
		{
			return false;
		}
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
	return true;
}

void Application::Update(float deltaTime)
{
	player_.Update(deltaTime);
	monster_.Update(deltaTime, player_.GetX());
	UpdatePlayerAnimation(deltaTime);
	UpdateMonsterAnimation(deltaTime);
	// Render()에서 범용 DrawSprite()를 사용하여 UV를 갱신 및 Draw
	// UpdateSpriteUV()는 더 이상 필요 없음
	// UpdateSpriteUV();
}

void Application::Render()
{
	// BackBuffer를 남청색으로 초기화
	context_->OMSetRenderTargets(1, renderTargetView_.GetAddressOf(), nullptr);
	const float clearColor[4] = { 0.1f, 0.15f, 0.25f, 1.0f };
	context_->ClearRenderTargetView(renderTargetView_.Get(), clearColor);

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

	// 현재 State에 맞는 Player Texture 가져오기
	ID3D11ShaderResourceView* currentTexture = GetCurrentPlayerTexture();

	// Pixel Shader Texture slot 0에 Shader Resource 연결
	context_->PSSetShaderResources(0, 1, &currentTexture);
	// Pixel Shader Texture slot 0에 Sampler 연결
	context_->PSSetSamplers(0, 1, samplerState_.GetAddressOf());

	// Blend State 적용
	context_->OMSetBlendState(blendState_.Get(), nullptr, 0xFFFFFFFF);

	// Draw
	DrawBackground();
	DrawTrees();
	DrawGround();
	DrawMonster();
	DrawPlayer();

	// BackBuffer 출력
	swapChain_->Present(1, 0);
}

void Application::UpdatePlayerAnimation(float deltaTime)
{
	const PlayerState currentState = player_.GetState();

	// PlayerState가 바뀌면 animationTimer와 currentFrame 초기화
	if (currentState != previousPlayerState_)
	{
		currentFrame_ = 0;
		animationTimer_ = 0.0f;
		previousPlayerState_ = currentState;
	}

	// Animation
	// deltaTime 만큼 animationTimer 증가
	animationTimer_ += deltaTime;
	// 0.15초 마다 frame 갱신
	constexpr float frameDuration = 0.15f;
	if (animationTimer_ >= frameDuration)
	{
		animationTimer_ -= frameDuration;
		// frame 증가
		const int frameCount = GetCurrentFrameCount();
		currentFrame_ = (currentFrame_ + 1) % frameCount;
	}
}

// UpdatePlayerAnimation과 동일하게 구성
void Application::UpdateMonsterAnimation(float deltaTime)
{
	const MonsterState state = monster_.GetState();

	if (state != previousMonsterState_)
	{
		monsterCurrentFrame_ = 0;
		monsterAnimationTimer_ = 0.0f;

		previousMonsterState_ = state;
	}

	monsterAnimationTimer_ += deltaTime;

	constexpr float frameDuration = 0.15f;

	if (monsterAnimationTimer_ < frameDuration) return;

	monsterAnimationTimer_ -= frameDuration;
	const int frameCount = GetCurrentMonsterFrameCount();

	switch (state)
	{
	case MonsterState::Idle:
	case MonsterState::Chase:
		monsterCurrentFrame_ = (monsterCurrentFrame_ + 1) % frameCount;
		break;

	case MonsterState::Hit:
		if (monsterCurrentFrame_ < frameCount - 1) monsterCurrentFrame_++;
		else monster_.FinishHit();
		break;

	case MonsterState::Dead:
		if (monsterCurrentFrame_ < frameCount - 1) monsterCurrentFrame_++;
		break;
	}
}

bool Application::IsMonsterAnimationFinished() const
{
	return monsterCurrentFrame_ >= GetCurrentMonsterFrameCount() - 1;
}


// 화면 전체에 배경 draw
void Application::DrawBackground()
{
	DrawSprite(
		backgroundTextureView_.Get(),
		0.0f,
		0.0f,
		1.0f,
		1.0f
	);
}

void Application::DrawTrees()
{
	constexpr float groundY = -0.3f;

	constexpr float treeHalfWidth = 0.1f;
	constexpr float treeHalfHeight = 0.34f;

	constexpr float treeCenterY = groundY + treeHalfHeight;


	// 왼쪽, 오른쪽에 나무 그리기
	DrawSprite(
		treeTextureView_.Get(),
		-0.65f,
		treeCenterY,
		treeHalfWidth,
		treeHalfHeight
	);

	DrawSprite(
		treeTextureView_.Get(),
		0.6f,
		treeCenterY,
		treeHalfWidth,
		treeHalfHeight
	);
}

void Application::DrawGround()
{
	constexpr float groundY = -0.3f;

	constexpr float halfWidth = 0.1f;
	constexpr float halfHeight = 0.1f;

	constexpr float centerY = groundY - halfHeight;

	constexpr int groundCount = 15;

	// groundCount만큼 groundTextureView_를 반복해서 그리기
	for (int i = 0; i < groundCount; ++i)
	{
		const float x = -1.0f + static_cast<float>(i) * halfWidth * 1.5f;

		DrawSprite(
			groundTextureView_.Get(),
			x,
			centerY,
			halfWidth,
			halfHeight
		);
	}
}

void Application::DrawPlayer()
{
	const float frameCount = static_cast<float>(GetCurrentFrameCount());
	const float frameWidth = 1.0f / frameCount;

	const float u0 = currentFrame_ * frameWidth;
	const float u1 = u0 + frameWidth;

	constexpr float renderOffsetY = -0.07f; // player sprite를 ground에 맞추기 위해 y offset 적용

	DrawSprite(
		GetCurrentPlayerTexture(),
		player_.GetX(),
		player_.GetY() + renderOffsetY,
		0.1f,
		0.18f,
		u0,
		0.0f,
		u1,
		1.0f,
		!player_.IsFacingRight()
	);
}

void Application::DrawMonster()
{
	const float frameCount = static_cast<float>(GetCurrentMonsterFrameCount());
	const float frameWidth = 1.0f / frameCount;

	const float u0 = monsterCurrentFrame_ * frameWidth;
	const float u1 = u0 + frameWidth;

	constexpr float renderOffsetY = -0.04f;

	DrawSprite(
		GetCurrentMonsterTexture(),
		monster_.GetX(),
		monster_.GetY() + renderOffsetY,
		0.08f,
		0.08f,
		u0,
		0.0f,
		u1,
		1.0f,
		monster_.IsFacingRight()
	);
}


//void Application::UpdateSpriteUV()
//{
//	// frame 수에 따라 frame width를 계산
//	const float frameWidth = 1.0f / static_cast<float>(GetCurrentFrameCount());
//
//	// frame width에 따라서 u0, u1 계산
//	const float u0 = currentFrame_ * frameWidth;
//	const float u1 = u0 + frameWidth;
//
//	const float halfWidth = 0.1f;
//	const float halfHeight = 0.18f;
//
//	// facingRight_에 따라 u0, u1을 좌우 반전
//	float leftU = player_.IsFacingRight() ? u0 : u1;
//	float rightU = player_.IsFacingRight() ? u1 : u0;
//
//	const  float playerX = player_.GetX();
//	const float playerY = player_.GetY();
//	// playerX_, playerY_를 중심으로 transformation 적용된 vertex 좌표 계산
//	Vertex vertices[] =
//	{
//		{ playerX - halfWidth, playerY - halfHeight, 0.0f, leftU, 1.0f },
//		{ playerX + halfWidth, playerY - halfHeight, 0.0f, rightU, 1.0f },
//		{ playerX + halfWidth, playerY + halfHeight, 0.0f, rightU, 0.0f },
//		{ playerX - halfWidth, playerY + halfHeight, 0.0f, leftU, 0.0f }
//	};
//
//	D3D11_MAPPED_SUBRESOURCE mappedResource{};
//
//	// CPU가 GPU Resource에 데이터를 쓸 수 있도록 접근 가능한 메모리 영역 요구
//	HRESULT hr = context_->Map(
//		vertexBuffer_.Get(),
//		0,
//		D3D11_MAP_WRITE_DISCARD,
//		0,
//		&mappedResource
//	);
//
//	if (FAILED(hr)) return;
//
//	// 새 vertex 데이터 넣기
//	memcpy(mappedResource.pData, vertices, sizeof(vertices));
//
//	// CPU 작업 끝, GPU가 Resource 사용
//	context_->Unmap(vertexBuffer_.Get(), 0);
//}

// UpdateSpriteUV()를 범용 DrawSprite()로 계승
// Sprite 하나 그릴 때마다 Vertex Buffer를 갱신하고 즉시 Draw
void Application::DrawSprite(
	ID3D11ShaderResourceView* textureView,
	float x,
	float y,
	float halfWidth,
	float halfHeight,
	float u0,
	float v0,
	float u1,
	float v1,
	bool flipX)
{
	float leftU = flipX ? u1 : u0;
	float rightU = flipX ? u0 : u1;

	const float vertices[] =
	{
		x - halfWidth, y - halfHeight, 0.0f, leftU, v1,
		x + halfWidth, y - halfHeight, 0.0f, rightU, v1,
		x + halfWidth, y + halfHeight, 0.0f, rightU, v0,
		x - halfWidth, y + halfHeight, 0.0f, leftU, v0
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
}

float Application::GetDeltaTime()
{
	auto currentTime = steady_clock::now();

	// deltaTime 계산
	float deltaTime = duration<float>(currentTime - previousTime_).count();

	// previousTime 갱신
	previousTime_ = currentTime;

	return deltaTime;
}

int Application::GetCurrentFrameCount() const
{
	switch (player_.GetState())
	{
	case PlayerState::Idle:
		return kIdleFrameCount;
	case PlayerState::Run:
		return kRunFrameCount;
	case PlayerState::JumpStart:
		return kJumpStartFrameCount;
	case PlayerState::JumpEnd:
		return kJumpEndFrameCount;
	}

	return 1;
}

int Application::GetCurrentMonsterFrameCount() const
{
	switch (monster_.GetState())
	{
	case MonsterState::Idle:
		return kMonsterIdleFrameCount;
	case MonsterState::Chase:
		return kMonsterChaseFrameCount;
	case MonsterState::Hit:
		return kMonsterHitFrameCount;
	case MonsterState::Dead:
		return kMonsterDeadFrameCount;
	}
}

ID3D11ShaderResourceView* Application::GetCurrentPlayerTexture() const
{
	switch (player_.GetState())
	{
	case PlayerState::Idle:
		return idleTextureView_.Get();
	case PlayerState::Run:
		return runTextureView_.Get();
	case PlayerState::JumpStart:
		return jumpStartTextureView_.Get();
	case PlayerState::JumpEnd:
		return jumpEndTextureView_.Get();
	}
	return nullptr;
}

ID3D11ShaderResourceView* Application::GetCurrentMonsterTexture() const
{
	switch (monster_.GetState())
	{
	case MonsterState::Idle:
		return monsterIdleTextureView_.Get();
	case MonsterState::Chase:
		return monsterChaseTextureView_.Get();
	case MonsterState::Hit:
		return monsterHitTextureView_.Get();
	case MonsterState::Dead:
		return monsterDeadTextureView_.Get();
	}
	return nullptr;
}