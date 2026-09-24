#include "Application.h"
#include <algorithm>
#include <cwchar>

// Image 디코더
#include "../ThirdParty/stb/stb_image.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

using namespace std::chrono;

// Windows -> Message Queue -> WindowProc
namespace
{
	LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
			if (wParam == VK_MENU) return 0;
			break;
		case WM_SYSCOMMAND:
			if ((wParam & 0xFF0) == SC_KEYMENU) return 0;
			break;
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

	if (!resourceManager_.Initialize(device_.Get())) return false;

	if (!renderer_.Initialize(
		device_.Get(),
		context_.Get(),
		&resourceManager_,
		&profiler_,
		kWindowSize))
	{
		return false;
	}

	gameWorld_.Initialize();

	previousTime_ = steady_clock::now();

	return true;
}

// Game Loop
int Application::Run()
{
	MSG message{};

	while (true)
	{
		// Message Pump부터 Present까지 한 프레임 바깥 구간을 측정한다.
		profiler_.BeginFrame();
		bool shouldContinue = false;
		{
			// 메시지 큐 확인에 걸린 시간도 전체 프레임과 별도 집계한다.
			ProfileScope scope(profiler_, ProfileCategory::MessagePump);
			shouldContinue = ProcessMessages();
		}
		if (!shouldContinue) break;

		//게임 상태 갱신
		const float deltaTime = GetDeltaTime();
		Update(deltaTime);
		//화면 그리기
		Render();
		profiler_.EndFrame();

		// 표시 갱신은 측정 구간 밖에서 제한된 주기로 실행한다.
		UpdateProfilerWindowTitle(deltaTime);
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

	// 지정한 게임 화면 크기로 창 생성
	RECT windowRect{
		0,
		0,
		static_cast<LONG>(kWindowSize.x),
		static_cast<LONG>(kWindowSize.y)
	};

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

	desc.BufferDesc.Width = static_cast<UINT>(kWindowSize.x);
	desc.BufferDesc.Height = static_cast<UINT>(kWindowSize.y);
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

	viewport.Width = kWindowSize.x;
	viewport.Height = kWindowSize.y;

	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	context_->RSSetViewports(1, &viewport);

	return true;
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
	// 이 상위 시간에는 Entity Update, Physics, Collision 하위 시간이 포함된다.
	ProfileScope scope(profiler_, ProfileCategory::Update);
	// player, enemy관련 전부 gameWorld로 이관
	gameWorld_.Update(deltaTime, profiler_);
}

void Application::Render()
{
	{
		// Render CPU 시간은 Present의 VSync 대기와 분리해서 기록한다.
		ProfileScope renderScope(profiler_, ProfileCategory::Render);

		// BackBuffer를 남청색으로 초기화
		context_->OMSetRenderTargets(1, renderTargetView_.GetAddressOf(), nullptr);
		const float clearColor[4] = { 0.1f, 0.15f, 0.25f, 1.0f };
		context_->ClearRenderTargetView(renderTargetView_.Get(), clearColor);

		renderer_.Begin();

		// 모든 GameObject를 생성 순서대로 렌더링
		for (const auto& object : gameWorld_.GetGameObjects())
		{
			if (!object->IsActive()) continue;

			const RenderInfo info = object->GetRenderInfo();
			if (!info.visible) continue;

			renderer_.Draw(info);
		}

		RenderUI();
	}

	// BackBuffer 출력
	{
		// Present(1, 0)는 VSync 대기를 포함할 수 있어 별도 항목으로 측정한다.
		ProfileScope presentScope(profiler_, ProfileCategory::Present);
		swapChain_->Present(1, 0);
	}
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

void Application::UpdateProfilerWindowTitle(float deltaTime)
{
	profilerTitleTimer_ += deltaTime;
	if (profilerTitleTimer_ < 0.5f)
	{
		return;
	}
	profilerTitleTimer_ = 0.0f;

	// 창 제목을 간단한 런타임 측정 표시로 사용하며 게임 렌더 흐름에는 개입하지 않는다.
	const ProfileSnapshot average = profiler_.GetAverage();
	const double collisionMilliseconds =
		average.GetMilliseconds(ProfileCategory::GroundCollision)
		+ average.GetMilliseconds(ProfileCategory::CombatCollision)
		+ average.GetMilliseconds(ProfileCategory::ItemCollision);
	wchar_t title[384]{};
	swprintf_s(
		title,
		L"DX11ScrollRPG | %.1f FPS | Frame %.2f ms | Update %.2f ms | Physics %.2f ms | Collision %.2f ms | Render %.2f ms | Present %.2f ms | Entity %.0f | Sprite %.0f | Draw %.0f | AABB %.0f",
		average.GetAverageFPS(),
		average.GetMilliseconds(ProfileCategory::Frame),
		average.GetMilliseconds(ProfileCategory::Update),
		average.GetMilliseconds(ProfileCategory::Physics),
		collisionMilliseconds,
		average.GetMilliseconds(ProfileCategory::Render),
		average.GetMilliseconds(ProfileCategory::Present),
		average.GetCounter(ProfileCounter::ActiveEntities),
		average.GetCounter(ProfileCounter::SpriteDraws),
		average.GetCounter(ProfileCounter::DrawCalls),
		average.GetCounter(ProfileCounter::CollisionChecks));
	SetWindowText(hwnd_, title);
}

void Application::RenderUI()
{
	Player* player = gameWorld_.GetPlayer();

	if (!player)return;

	for (int i = 0; i < player->GetHP(); ++i)
	{
		RenderInfo heart;
		heart.spriteId = SpriteId::Heart;
		heart.position = { -0.9f + i * 0.1f, 0.88f };
		heart.frameSizePixels = { 1254,1254 };
		heart.renderHalfSize = { 0.0f, 0.08f };

		renderer_.Draw(heart);
	}

	if (player->IsDead())
	{
		RenderInfo gameOver;
		gameOver.spriteId = SpriteId::GameOver;
		gameOver.position = { 0.0f, 0.1f };
		gameOver.frameSizePixels = { 1921, 819 };
		gameOver.renderHalfSize = { 0.0f, 0.2f };

		renderer_.Draw(gameOver);
	}
}
