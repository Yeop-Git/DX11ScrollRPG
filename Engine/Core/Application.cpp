#include "Application.h"
#include <algorithm>
#include <cmath>
#include <cwchar>
#include <windowsx.h>

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
		case WM_NCCREATE:
		{
			// CreateWindowEx에서 받은 Application 포인터를 이후 키·마우스 입력에 사용한다.
			const auto* createInfo = reinterpret_cast<const CREATESTRUCTW*>(lParam);
			SetWindowLongPtrW(
				hwnd,
				GWLP_USERDATA,
				reinterpret_cast<LONG_PTR>(createInfo->lpCreateParams));
			break;
		}
		case WM_KEYDOWN:
		{
			// 키 자동 반복에는 토글하지 않고 F1을 처음 누른 메시지만 처리한다.
			if (wParam == VK_F1 && (lParam & (1LL << 30)) == 0)
			{
				auto* application = reinterpret_cast<Application*>(
					GetWindowLongPtrW(hwnd, GWLP_USERDATA));
				if (application != nullptr)
				{
					application->ToggleProfilerPanel();
					return 0;
				}
			}
			break;
		}
		case WM_LBUTTONDOWN:
		{
			auto* application = reinterpret_cast<Application*>(
				GetWindowLongPtrW(hwnd, GWLP_USERDATA));
			if (application != nullptr
				&& application->HandleUIMouseDown(
					GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
			{
				return 0;
			}
			break;
		}
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

	// UIManager는 Renderer가 사용할 Direct3D Device와 Context를 빌려 쓴다.
	if (!uiManager_.Initialize(device_.Get(), context_.Get(), kWindowSize)) return false;

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

		if (const auto requestedTest = uiManager_.TakeRequestedStressTest())
		{
			// 버튼 요청은 프레임 완료 후 적용해 생성 비용이 측정값에 섞이지 않게 한다.
			selectedStressMode_ = requestedTest->mode;
			if (requestedTest->count == 0)
			{
				gameWorld_.SetStressTestMonsterCount(0);
				renderStressSprites_.clear();
				renderStressSpriteCount_ = 0;
			}
			else if (selectedStressMode_ == StressTestMode::MonsterEntities)
			{
				renderStressSprites_.clear();
				renderStressSpriteCount_ = 0;
				gameWorld_.SetStressTestMonsterCount(requestedTest->count);
			}
			else
			{
				gameWorld_.SetStressTestMonsterCount(0);
				RebuildRenderStressSprites(requestedTest->count);
			}
			gameWorld_.SetCombatDamageEnabled(GetActiveStressTestCount() == 0);
			profiler_.ResetHistory();
		}
		if (const auto requestedDepthTest = uiManager_.TakeRequestedDepthTestEnabled())
		{
			// 깊이 ON/OFF 측정 구간을 섞지 않도록 적용과 함께 Profiler 평균을 초기화한다.
			renderer_.SetDepthBufferEnabled(*requestedDepthTest);
			profiler_.ResetHistory();
		}
		if (const auto requestedBatching = uiManager_.TakeRequestedBatchingEnabled())
		{
			renderer_.SetBatchingEnabled(*requestedBatching);
			profiler_.ResetHistory();
		}
		if (const auto requestedQueue = uiManager_.TakeRequestedRenderQueueEnabled())
		{
			renderer_.SetRenderQueueEnabled(*requestedQueue);
			profiler_.ResetHistory();
		}
		if (const auto requestedMode = uiManager_.TakeRequestedStressMode())
		{
			selectedStressMode_ = *requestedMode;
		}

		UpdateUI(deltaTime);
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
		this
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

	// BackBuffer와 같은 크기의 깊이 버퍼가 픽셀별 앞뒤 관계를 보관한다.
	D3D11_TEXTURE2D_DESC depthDesc{};
	depthDesc.Width = static_cast<UINT>(kWindowSize.x);
	depthDesc.Height = static_cast<UINT>(kWindowSize.y);
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = 1;
	depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	hr = device_->CreateTexture2D(&depthDesc, nullptr, depthStencilTexture_.GetAddressOf());
	if (FAILED(hr)) return false;
	hr = device_->CreateDepthStencilView(
		depthStencilTexture_.Get(), nullptr, depthStencilView_.GetAddressOf());
	if (FAILED(hr)) return false;

	// RenderTarget 연결
	context_->OMSetRenderTargets(
		1, renderTargetView_.GetAddressOf(), depthStencilView_.Get());

	// Viewport 생성
	D3D11_VIEWPORT viewport{};

	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	viewport.Width = kWindowSize.x;
	viewport.Height = kWindowSize.y;

	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	context_->RSSetViewports(1, &viewport);

	// GPU 쿼리는 진단 기능이므로 생성 실패가 게임 초기화를 막지는 않는다.
	CreateGpuProfilerQueries();

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
	// 완료된 이전 프레임 GPU 결과만 확인하고, 준비 전이면 CPU를 기다리지 않는다.
	CollectGpuProfilerSamples();
	{
		// Scene Render CPU 시간을 UI 및 Present 대기와 분리해서 기록한다.
		ProfileScope renderScope(profiler_, ProfileCategory::Render);

		// BackBuffer를 남청색으로 초기화
		const float clearColor[4] = { 0.1f, 0.15f, 0.25f, 1.0f };
		context_->ClearRenderTargetView(renderTargetView_.Get(), clearColor);
		context_->OMSetRenderTargets(
			1, renderTargetView_.GetAddressOf(), depthStencilView_.Get());
		context_->ClearDepthStencilView(
			depthStencilView_.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		if (uiManager_.IsProfilerVisible())
		{
			BeginGpuProfilerSample();
		}
		renderer_.Begin(renderer_.IsDepthBufferEnabled(), renderer_.IsRenderQueueEnabled());

		const auto& renderLayers = gameWorld_.GetRenderLayers();
		const auto drawLayer = [this](
			const auto& layer,
			bool reverse,
			std::size_t layerIndex,
			bool reorderSafe)
		{
			auto drawObject = [this, layerIndex, reorderSafe](const RenderObject& renderObject)
			{
				if (renderObject.object == nullptr || !renderObject.object->IsActive()) return;

				RenderInfo info = renderObject.object->GetRenderInfo();
				if (!info.visible) return;
				info.depth = renderObject.depth;
				renderer_.Draw(info, layerIndex, reorderSafe);
			};

			if (reverse)
			{
				for (auto object = layer.rbegin(); object != layer.rend(); ++object)
				{
					drawObject(*object);
				}
			}
			else
			{
				for (const RenderObject& renderObject : layer)
				{
					drawObject(renderObject);
				}
			}
		};

		if (renderStressSpriteCount_ > 0)
		{
			const std::size_t backgroundLayer = static_cast<std::size_t>(RenderLayer::Background);
			drawLayer(renderLayers[backgroundLayer], false, backgroundLayer,
				renderer_.IsDepthBufferEnabled());
			const std::size_t stressLayer = static_cast<std::size_t>(RenderLayer::Count);
			for (const RenderInfo& sprite : renderStressSprites_)
			{
				renderer_.Draw(sprite, stressLayer, true);
			}
		}
		else if (renderer_.IsDepthBufferEnabled())
		{
			// 깊이 테스트를 쓸 때는 가까운 우선순위부터 그려 불필요한 픽셀 작업을 막는다.
			for (std::size_t layerIndex = 0; layerIndex < renderLayers.size(); ++layerIndex)
			{
				if (layerIndex == static_cast<std::size_t>(RenderLayer::TransparentItem))
				{
					// 투명 요청은 고정 순서가 필요하므로 불투명 Queue를 먼저 제출한다.
					renderer_.FlushRenderQueue();
					drawLayer(renderLayers[layerIndex], false, layerIndex, false);
				}
				else
				{
					drawLayer(renderLayers[layerIndex], false, layerIndex, true);
				}
			}
		}
		else
		{
			// 깊이 테스트가 없을 때는 같은 깊이 우선순위가 되도록 먼 항목부터 덮어 그린다.
			drawLayer(renderLayers[static_cast<std::size_t>(RenderLayer::Background)], false,
				static_cast<std::size_t>(RenderLayer::Background), false);
			drawLayer(renderLayers[static_cast<std::size_t>(RenderLayer::Environment)], true,
				static_cast<std::size_t>(RenderLayer::Environment), false);
			drawLayer(renderLayers[static_cast<std::size_t>(RenderLayer::TransparentItem)], false,
				static_cast<std::size_t>(RenderLayer::TransparentItem), false);
			drawLayer(renderLayers[static_cast<std::size_t>(RenderLayer::Monster)], true,
				static_cast<std::size_t>(RenderLayer::Monster), false);
			drawLayer(renderLayers[static_cast<std::size_t>(RenderLayer::Player)], true,
				static_cast<std::size_t>(RenderLayer::Player), false);
		}
		renderer_.FlushRenderQueue();

		// Scene의 마지막 묶음 제출도 Scene Render 측정에 포함한다.
		renderer_.Flush();
		EndGpuProfilerSample();

	}
	{
		// HUD와 Profiler는 같은 Renderer를 사용하되 Scene 비용과 별도로 잰다.
		ProfileScope uiScope(profiler_, ProfileCategory::UIRender);
		context_->OMSetRenderTargets(1, renderTargetView_.GetAddressOf(), nullptr);
		renderer_.Begin(false);
		uiManager_.Render(renderer_);
		// HUD와 Profiler의 마지막 묶음을 UI Render 구간 안에서 제출한다.
		renderer_.Flush();
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

void Application::ToggleProfilerPanel()
{
	uiManager_.ToggleProfilerPanel();
}

bool Application::CreateGpuProfilerQueries()
{
	D3D11_QUERY_DESC queryDesc{};
	for (GpuProfilerQuerySet& querySet : gpuProfilerQueries_)
	{
		queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
		if (FAILED(device_->CreateQuery(&queryDesc, querySet.disjoint.GetAddressOf())))
		{
			return false;
		}

		queryDesc.Query = D3D11_QUERY_TIMESTAMP;
		if (FAILED(device_->CreateQuery(&queryDesc, querySet.startTimestamp.GetAddressOf())))
		{
			return false;
		}
		if (FAILED(device_->CreateQuery(&queryDesc, querySet.endTimestamp.GetAddressOf())))
		{
			return false;
		}

		queryDesc.Query = D3D11_QUERY_PIPELINE_STATISTICS;
		if (FAILED(device_->CreateQuery(
			&queryDesc,
			querySet.pipelineStatistics.GetAddressOf())))
		{
			return false;
		}
	}

	gpuProfilerQueriesAvailable_ = true;
	return true;
}

void Application::CollectGpuProfilerSamples()
{
	if (!gpuProfilerQueriesAvailable_)
	{
		return;
	}

	for (GpuProfilerQuerySet& querySet : gpuProfilerQueries_)
	{
		if (!querySet.pending)
		{
			continue;
		}

		D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint{};
		D3D11_QUERY_DATA_PIPELINE_STATISTICS statistics{};
		UINT64 startTimestamp = 0;
		UINT64 endTimestamp = 0;
		const UINT getDataFlags = D3D11_ASYNC_GETDATA_DONOTFLUSH;

		const HRESULT disjointResult = context_->GetData(
			querySet.disjoint.Get(), &disjoint, sizeof(disjoint), getDataFlags);
		const HRESULT startResult = context_->GetData(
			querySet.startTimestamp.Get(),
			&startTimestamp,
			sizeof(startTimestamp),
			getDataFlags);
		const HRESULT endResult = context_->GetData(
			querySet.endTimestamp.Get(),
			&endTimestamp,
			sizeof(endTimestamp),
			getDataFlags);
		const HRESULT statisticsResult = context_->GetData(
			querySet.pipelineStatistics.Get(),
			&statistics,
			sizeof(statistics),
			getDataFlags);

		if (FAILED(disjointResult)
			|| FAILED(startResult)
			|| FAILED(endResult)
			|| FAILED(statisticsResult))
		{
			querySet.pending = false;
			continue;
		}
		if (disjointResult == S_FALSE
			|| startResult == S_FALSE
			|| endResult == S_FALSE
			|| statisticsResult == S_FALSE)
		{
			continue;
		}

		querySet.pending = false;
		if (disjoint.Disjoint
			|| disjoint.Frequency == 0
			|| endTimestamp < startTimestamp)
		{
			continue;
		}

		// 설정 변경 직전에 제출된 샘플은 새 평균에 섞지 않는다.
		if (querySet.depthTestEnabled != renderer_.IsDepthBufferEnabled()
			|| querySet.batchingEnabled != renderer_.IsBatchingEnabled()
			|| querySet.renderQueueEnabled != renderer_.IsRenderQueueEnabled()
			|| querySet.stressMonsterCount != gameWorld_.GetStressTestMonsterCount()
			|| querySet.renderStressSpriteCount != renderStressSpriteCount_
			|| querySet.stressTestMode != GetActiveStressTestMode())
		{
			continue;
		}

		const double gpuMilliseconds =
			static_cast<double>(endTimestamp - startTimestamp)
			* 1000.0 / static_cast<double>(disjoint.Frequency);
		profiler_.AddGpuSample(gpuMilliseconds, statistics.PSInvocations);
	}
}

void Application::BeginGpuProfilerSample()
{
	activeGpuProfilerQuery_ = nullptr;
	if (!gpuProfilerQueriesAvailable_)
	{
		return;
	}

	for (std::size_t offset = 0; offset < gpuProfilerQueries_.size(); ++offset)
	{
		const std::size_t index = (nextGpuProfilerQuery_ + offset)
			% gpuProfilerQueries_.size();
		GpuProfilerQuerySet& querySet = gpuProfilerQueries_[index];
		if (querySet.pending)
		{
			continue;
		}

		querySet.pending = true;
		querySet.depthTestEnabled = renderer_.IsDepthBufferEnabled();
		querySet.batchingEnabled = renderer_.IsBatchingEnabled();
		querySet.renderQueueEnabled = renderer_.IsRenderQueueEnabled();
		querySet.stressMonsterCount = gameWorld_.GetStressTestMonsterCount();
		querySet.renderStressSpriteCount = renderStressSpriteCount_;
		querySet.stressTestMode = GetActiveStressTestMode();
		activeGpuProfilerQuery_ = &querySet;
		nextGpuProfilerQuery_ = (index + 1) % gpuProfilerQueries_.size();
		context_->Begin(querySet.disjoint.Get());
		context_->Begin(querySet.pipelineStatistics.Get());
		context_->End(querySet.startTimestamp.Get());
		return;
	}
	// 네 프레임 분량의 쿼리가 모두 미완료면 해당 프레임 측정만 건너뛴다.
}

void Application::EndGpuProfilerSample()
{
	if (activeGpuProfilerQuery_ == nullptr)
	{
		return;
	}

	context_->End(activeGpuProfilerQuery_->endTimestamp.Get());
	context_->End(activeGpuProfilerQuery_->pipelineStatistics.Get());
	context_->End(activeGpuProfilerQuery_->disjoint.Get());
	activeGpuProfilerQuery_ = nullptr;
}

bool Application::HandleUIMouseDown(int x, int y)
{
	return uiManager_.HandleMouseDown(x, y);
}

void Application::UpdateUI(float deltaTime)
{
	UIFrameData frameData{};
	if (Player* player = gameWorld_.GetPlayer())
	{
		frameData.playerHp = player->GetHP();
		frameData.playerDead = player->IsDead();
	}
	frameData.stressTestCount = GetActiveStressTestCount();
	frameData.selectedStressMode = selectedStressMode_;
	frameData.depthTestEnabled = renderer_.IsDepthBufferEnabled();
	frameData.batchingEnabled = renderer_.IsBatchingEnabled();
	frameData.renderQueueEnabled = renderer_.IsRenderQueueEnabled();
	frameData.gpuMetricsAvailable = gpuProfilerQueriesAvailable_;
	frameData.profiler = profiler_.GetAverage();
	uiManager_.Update(deltaTime, frameData);
}

void Application::RebuildRenderStressSprites(std::size_t spriteCount)
{
	renderStressSprites_.clear();
	renderStressSprites_.reserve(spriteCount);
	renderStressSpriteCount_ = spriteCount;
	if (spriteCount == 0)
	{
		return;
	}

	// 균일한 비중첩 Grid로 Update/Collision을 거치지 않는 렌더 전용 부하를 만든다.
	const float aspect = kWindowSize.x / kWindowSize.y;
	const std::size_t columns = static_cast<std::size_t>(std::ceil(
		std::sqrt(static_cast<double>(spriteCount) * aspect)));
	const std::size_t rows = (spriteCount + columns - 1) / columns;
	const float cellWidth = 2.0f / static_cast<float>(columns);
	const float cellHeight = 2.0f / static_cast<float>(rows);
	for (std::size_t index = 0; index < spriteCount; ++index)
	{
		RenderInfo info{};
		info.spriteId = index % 2 == 0 ? SpriteId::PlayerIdle : SpriteId::MonsterIdle;
		info.position = {
			-1.0f + (static_cast<float>(index % columns) + 0.5f) * cellWidth,
			1.0f - (static_cast<float>(index / columns) + 0.5f) * cellHeight
		};
		info.renderHalfSize = { cellWidth * 0.46f, cellHeight * 0.46f };
		info.frameCount = 4;
		info.frameSizePixels = index % 2 == 0
			? Vector2{ 64.0f, 80.0f }
			: Vector2{ 48.0f, 32.0f };
		info.depth = 0.5f;
		info.renderMode = SpriteRenderMode::Cutout;
		renderStressSprites_.push_back(info);
	}
}

std::size_t Application::GetActiveStressTestCount() const
{
	return renderStressSpriteCount_ > 0
		? renderStressSpriteCount_
		: gameWorld_.GetStressTestMonsterCount();
}

StressTestMode Application::GetActiveStressTestMode() const
{
	return renderStressSpriteCount_ > 0
		? StressTestMode::AlternatingSprites
		: StressTestMode::MonsterEntities;
}
