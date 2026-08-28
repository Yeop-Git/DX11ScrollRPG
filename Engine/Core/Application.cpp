#include "Application.h"

// HLSL 컴파일러
#include <d3dcompiler.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// 정점 structure
struct Vertex
{
	float x, y, z;
	float r, g, b, a;
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
		Update();
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
	// 사각형 vertex 데이터
	Vertex vertices[] =
	{
		{ -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f },
		{ 0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f },
		{ 0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f },
		{ -0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f }
	};

	// vertex 버퍼 생성
	D3D11_BUFFER_DESC bufferDesc{};
	bufferDesc.ByteWidth = sizeof(vertices);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

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
		{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
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

void Application::Update()
{
	// Update logic here 
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

	// Draw
	context_->DrawIndexed(6, 0, 0);

	// BackBuffer 출력
	swapChain_->Present(1, 0);
}