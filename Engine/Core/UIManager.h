#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>

#include <cstddef>
#include <optional>

#include "Profiler.h"
#include "../Graphics/Renderer.h"

using namespace Microsoft::WRL;

enum class StressTestMode
{
	MonsterEntities,
	AlternatingSprites
};

struct StressTestRequest
{
	std::size_t count = 0;
	StressTestMode mode = StressTestMode::MonsterEntities;
};

// Application이 매 프레임 UI에 전달하는 읽기 전용 화면 데이터다.
struct UIFrameData
{
	int playerHp = 0;
	bool playerDead = false;
	std::size_t stressTestCount = 0;
	StressTestMode selectedStressMode = StressTestMode::MonsterEntities;
	bool depthTestEnabled = true;
	bool batchingEnabled = true;
	bool renderQueueEnabled = false;
	bool gpuMetricsAvailable = false;
	ProfileSnapshot profiler{};
};

// UIManager 내부 hit-test와 픽셀 좌표 배치에 사용하는 사각 영역이다.
struct UIPixelRect
{
	int x;
	int y;
	int width;
	int height;
};

// HUD와 Profiler 패널의 상태·입력을 관리하고 Renderer에 UI 그리기를 요청한다.
class UIManager
{
public:
	bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context, Vector2 viewportSize);

	// WindowProc에서 전달받아 패널을 토글하거나 Profiler 버튼 요청을 저장한다.
	void ToggleProfilerPanel();
	bool IsProfilerVisible() const { return profilerVisible_; }
	bool HandleMouseDown(int x, int y);
	std::optional<StressTestRequest> TakeRequestedStressTest();
	std::optional<bool> TakeRequestedDepthTestEnabled();
	std::optional<bool> TakeRequestedBatchingEnabled();
	std::optional<bool> TakeRequestedRenderQueueEnabled();
	std::optional<StressTestMode> TakeRequestedStressMode();

	// 프레임 상태를 저장하고 Profiler 표시 텍스처는 열린 동안만 0.5초마다 갱신한다.
	void Update(float deltaTime, const UIFrameData& frameData);
	// 게임 HUD와 Profiler 패널을 현재 BackBuffer에 렌더링한다.
	void Render(Renderer& renderer) const;

private:
	bool CreateTextTexture();
	bool UpdateTextTexture();
	bool DrawProfilerText(HDC memoryDc) const;
	void DrawProfilerStats(HDC memoryDc) const;
	bool UploadTextPixels(const void* pixels);
	Vector2 ToClipCenter(const UIPixelRect& rect) const;
	Vector2 ToClipHalfSize(const UIPixelRect& rect) const;
	void DrawProfilerPanel(Renderer& renderer) const;

private:
	// Direct3D Device/Context는 Application이 소유하고 이 클래스는 빌려 쓴다.
	ID3D11Device* device_ = nullptr;
	ID3D11DeviceContext* context_ = nullptr;
	Vector2 viewportSize_{ 1.0f, 1.0f };

	// 텍스트는 GDI로 투명 픽셀 마스크를 만들고 Renderer가 DX11 텍스처로 출력한다.
	ComPtr<ID3D11Texture2D> textTexture_;
	ComPtr<ID3D11ShaderResourceView> textTextureView_;

	UIFrameData frameData_{};
	std::optional<StressTestRequest> requestedStressTest_;
	std::optional<bool> requestedDepthTestEnabled_;
	std::optional<bool> requestedBatchingEnabled_;
	std::optional<bool> requestedRenderQueueEnabled_;
	std::optional<StressTestMode> requestedStressMode_;
	float profilerRefreshTimer_ = 0.0f;
	bool profilerVisible_ = false;
};
