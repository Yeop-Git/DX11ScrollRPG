#include "UIManager.h"

#include <cwchar>
#include <cstring>
#include <iterator>

#pragma comment(lib, "gdi32.lib")

namespace
{
	constexpr int kProfilerX = 20;
	constexpr int kProfilerY = 20;
	constexpr int kProfilerWidth = 500;
	constexpr int kProfilerHeight = 250;
	constexpr float kProfilerRefreshSeconds = 0.5f;

	struct ProfilerButton
	{
		UIPixelRect bounds;
		std::size_t stressMonsterCount;
		const wchar_t* label;
		RendererColor color;
	};

	// 버튼 크기, 화면 위치, 입력 결과, 표시 이름을 한 곳에서 관리한다.
	constexpr ProfilerButton kProfilerButtons[] =
	{
		{
			{ kProfilerX + 16, kProfilerY + 198, 72, 30 },
			100,
			L"100",
			{ 0.12f, 0.27f, 0.43f, 1.0f }
		},
		{
			{ kProfilerX + 96, kProfilerY + 198, 72, 30 },
			500,
			L"500",
			{ 0.12f, 0.27f, 0.43f, 1.0f }
		},
		{
			{ kProfilerX + 176, kProfilerY + 198, 72, 30 },
			1000,
			L"1000",
			{ 0.12f, 0.27f, 0.43f, 1.0f }
		},
		{
			{ kProfilerX + 256, kProfilerY + 198, 72, 30 },
			5000,
			L"5000",
			{ 0.12f, 0.27f, 0.43f, 1.0f }
		},
		{
			{ kProfilerX + 342, kProfilerY + 198, 120, 30 },
			0,
			L"Stop Test",
			{ 0.48f, 0.16f, 0.17f, 1.0f }
		}
	};

	bool Contains(const UIPixelRect& rect, int x, int y)
	{
		return x >= rect.x
			&& x < rect.x + rect.width
			&& y >= rect.y
			&& y < rect.y + rect.height;
	}

	HFONT CreateProfilerFont(int height, int weight, const wchar_t* fontName)
	{
		return CreateFontW(
			-height,
			0,
			0,
			0,
			weight,
			FALSE,
			FALSE,
			FALSE,
			DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			ANTIALIASED_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE,
			fontName);
	}
}

bool UIManager::Initialize(
	ID3D11Device* device,
	ID3D11DeviceContext* context,
	Vector2 viewportSize)
{
	device_ = device;
	context_ = context;
	viewportSize_ = viewportSize;

	if (device_ == nullptr || context_ == nullptr)
	{
		return false;
	}

	if (viewportSize_.x <= 0.0f || viewportSize_.y <= 0.0f)
	{
		return false;
	}

	if (!CreateTextTexture())
	{
		return false;
	}

	return UpdateTextTexture();
}

void UIManager::ToggleProfilerPanel()
{
	profilerVisible_ = !profilerVisible_;
	profilerRefreshTimer_ = 0.0f;

	if (profilerVisible_)
	{
		// 패널을 여는 즉시 현재 캐시를 글자 텍스처에 반영한다.
		UpdateTextTexture();
	}
}

bool UIManager::HandleMouseDown(int x, int y)
{
	if (!profilerVisible_)
	{
		return false;
	}

	for (const ProfilerButton& button : kProfilerButtons)
	{
		if (!Contains(button.bounds, x, y))
		{
			continue;
		}

		// Application이 프레임 측정을 마친 뒤 이 요청을 한 번 소비한다.
		requestedStressMonsterCount_ = button.stressMonsterCount;
		return true;
	}

	return false;
}

std::optional<std::size_t> UIManager::TakeRequestedStressMonsterCount()
{
	// 요청을 한 번만 소비해 같은 버튼 조작이 여러 프레임 실행되지 않게 한다.
	const auto request = requestedStressMonsterCount_;
	requestedStressMonsterCount_.reset();
	return request;
}

void UIManager::Update(float deltaTime, const UIFrameData& frameData)
{
	const bool stressCountChanged =
		frameData_.stressMonsterCount != frameData.stressMonsterCount;
	frameData_ = frameData;

	if (!profilerVisible_)
	{
		profilerRefreshTimer_ = 0.0f;
		return;
	}

	profilerRefreshTimer_ += deltaTime;
	if (!stressCountChanged && profilerRefreshTimer_ < kProfilerRefreshSeconds)
	{
		return;
	}

	profilerRefreshTimer_ = 0.0f;
	UpdateTextTexture();
}

void UIManager::Render(Renderer& renderer) const
{
	// 기존 Application HUD와 같은 Sprite API로 하트와 Game Over를 그린다.
	for (int heartIndex = 0; heartIndex < frameData_.playerHp; ++heartIndex)
	{
		RenderInfo heart;
		heart.spriteId = SpriteId::Heart;
		heart.position = { -0.9f + heartIndex * 0.1f, 0.88f };
		heart.frameSizePixels = { 1254.0f, 1254.0f };
		heart.renderHalfSize = { 0.0f, 0.08f };
		renderer.Draw(heart);
	}

	if (frameData_.playerDead)
	{
		RenderInfo gameOver;
		gameOver.spriteId = SpriteId::GameOver;
		gameOver.position = { 0.0f, 0.1f };
		gameOver.frameSizePixels = { 1921.0f, 819.0f };
		gameOver.renderHalfSize = { 0.0f, 0.2f };
		renderer.Draw(gameOver);
	}

	if (profilerVisible_)
	{
		DrawProfilerPanel(renderer);
	}
}

bool UIManager::CreateTextTexture()
{
	D3D11_TEXTURE2D_DESC textureDesc{};
	textureDesc.Width = kProfilerWidth;
	textureDesc.Height = kProfilerHeight;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DYNAMIC;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT result = device_->CreateTexture2D(
		&textureDesc,
		nullptr,
		textTexture_.GetAddressOf());
	if (FAILED(result))
	{
		return false;
	}

	result = device_->CreateShaderResourceView(
		textTexture_.Get(),
		nullptr,
		textTextureView_.GetAddressOf());

	return SUCCEEDED(result);
}

bool UIManager::UpdateTextTexture()
{
	// 글자 래스터화를 위한 임시 GDI 비트맵을 만들고 완료 후 바로 해제한다.
	HDC memoryDc = CreateCompatibleDC(nullptr);
	if (memoryDc == nullptr)
	{
		return false;
	}

	BITMAPINFO bitmapInfo{};
	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfo.bmiHeader.biWidth = kProfilerWidth;
	bitmapInfo.bmiHeader.biHeight = -kProfilerHeight;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;

	void* dibPixels = nullptr;
	HBITMAP dibBitmap = CreateDIBSection(
		memoryDc,
		&bitmapInfo,
		DIB_RGB_COLORS,
		&dibPixels,
		nullptr,
		0);
	if (dibBitmap == nullptr || dibPixels == nullptr)
	{
		if (dibBitmap != nullptr)
		{
			DeleteObject(dibBitmap);
		}

		DeleteDC(memoryDc);
		return false;
	}

	HGDIOBJ previousBitmap = SelectObject(memoryDc, dibBitmap);
	std::memset(dibPixels, 0, kProfilerWidth * kProfilerHeight * 4);
	SetBkMode(memoryDc, TRANSPARENT);
	SetTextColor(memoryDc, RGB(255, 255, 255));

	const bool textDrawn = DrawProfilerText(memoryDc);
	SelectObject(memoryDc, previousBitmap);

	bool uploaded = false;
	if (textDrawn)
	{
		uploaded = UploadTextPixels(dibPixels);
	}

	DeleteObject(dibBitmap);
	DeleteDC(memoryDc);
	return uploaded;
}

bool UIManager::DrawProfilerText(HDC memoryDc) const
{
	HFONT bodyFont = CreateProfilerFont(15, FW_NORMAL, L"Consolas");
	HFONT titleFont = CreateProfilerFont(19, FW_BOLD, L"Segoe UI");
	if (bodyFont == nullptr || titleFont == nullptr)
	{
		if (bodyFont != nullptr)
		{
			DeleteObject(bodyFont);
		}

		if (titleFont != nullptr)
		{
			DeleteObject(titleFont);
		}

		return false;
	}

	// 선택 전 폰트를 보관해 GDI DC의 상태를 원래대로 돌려놓는다.
	HGDIOBJ previousFont = SelectObject(memoryDc, titleFont);

	RECT titleRect{ 16, 10, kProfilerWidth - 16, 36 };
	DrawTextW(
		memoryDc,
		L"Runtime Profiler  [F1]",
		-1,
		&titleRect,
		DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);

	SelectObject(memoryDc, bodyFont);
	DrawProfilerStats(memoryDc);

	for (const ProfilerButton& button : kProfilerButtons)
	{
		RECT buttonTextRect
		{
			button.bounds.x - kProfilerX,
			button.bounds.y - kProfilerY,
			button.bounds.x - kProfilerX + button.bounds.width,
			button.bounds.y - kProfilerY + button.bounds.height
		};

		DrawTextW(
			memoryDc,
			button.label,
			-1,
			&buttonTextRect,
			DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	}

	SelectObject(memoryDc, previousFont);
	DeleteObject(bodyFont);
	DeleteObject(titleFont);
	return true;
}

void UIManager::DrawProfilerStats(HDC memoryDc) const
{
	const double collisionMilliseconds =
		frameData_.profiler.GetMilliseconds(ProfileCategory::GroundCollision)
		+ frameData_.profiler.GetMilliseconds(ProfileCategory::CombatCollision)
		+ frameData_.profiler.GetMilliseconds(ProfileCategory::ItemCollision);

	wchar_t stats[512]{};
	swprintf_s(
		stats,
		L"FPS %5.1f  Frame %6.2f ms  Present %6.2f ms\n"
		L"Update %6.2f ms  Physics %6.2f ms  Collision %6.2f ms\n"
		L"Render %6.2f ms  UI %6.2f ms\n"
		L"Entities %5.0f  Sprites %5.0f  Draw Calls %5.0f\n"
		L"AABB Checks %7.0f  Stress Overlaps %5.0f",
		frameData_.profiler.GetAverageFPS(),
		frameData_.profiler.GetMilliseconds(ProfileCategory::Frame),
		frameData_.profiler.GetMilliseconds(ProfileCategory::Present),
		frameData_.profiler.GetMilliseconds(ProfileCategory::Update),
		frameData_.profiler.GetMilliseconds(ProfileCategory::Physics),
		collisionMilliseconds,
		frameData_.profiler.GetMilliseconds(ProfileCategory::Render),
		frameData_.profiler.GetMilliseconds(ProfileCategory::UIRender),
		frameData_.profiler.GetCounter(ProfileCounter::ActiveEntities),
		frameData_.profiler.GetCounter(ProfileCounter::SpriteDraws),
		frameData_.profiler.GetCounter(ProfileCounter::DrawCalls),
		frameData_.profiler.GetCounter(ProfileCounter::CollisionChecks),
		frameData_.profiler.GetCounter(ProfileCounter::StressCollisionOverlaps));

	RECT statsRect{ 16, 42, kProfilerWidth - 16, 158 };
	DrawTextW(
		memoryDc,
		stats,
		-1,
		&statsRect,
		DT_LEFT | DT_TOP | DT_NOPREFIX);
}

bool UIManager::UploadTextPixels(const void* pixels)
{
	D3D11_MAPPED_SUBRESOURCE mapped{};
	// 텍스처가 이전 프레임에 SRV로 연결되어 있으면 갱신 전에 슬롯을 비운다.
	ID3D11ShaderResourceView* emptyView = nullptr;
	context_->PSSetShaderResources(0, 1, &emptyView);

	const HRESULT result = context_->Map(
		textTexture_.Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mapped);
	if (FAILED(result))
	{
		return false;
	}

	const auto* source = static_cast<const unsigned char*>(pixels);
	for (int y = 0; y < kProfilerHeight; ++y)
	{
		auto* destination = static_cast<unsigned char*>(mapped.pData)
			+ static_cast<std::size_t>(y) * mapped.RowPitch;
		const unsigned char* sourceRow = source
			+ static_cast<std::size_t>(y) * kProfilerWidth * 4;

		for (int x = 0; x < kProfilerWidth; ++x)
		{
			// RGB 중 가장 밝은 값을 글자 가장자리의 투명도(coverage)로 사용한다.
			unsigned char coverage = sourceRow[x * 4];
			if (sourceRow[x * 4 + 1] > coverage)
			{
				coverage = sourceRow[x * 4 + 1];
			}

			if (sourceRow[x * 4 + 2] > coverage)
			{
				coverage = sourceRow[x * 4 + 2];
			}

			destination[x * 4] = 255;
			destination[x * 4 + 1] = 255;
			destination[x * 4 + 2] = 255;
			destination[x * 4 + 3] = coverage;
		}
	}

	context_->Unmap(textTexture_.Get(), 0);
	return true;
}

Vector2 UIManager::ToClipCenter(const UIPixelRect& rect) const
{
	const float centerX =
		static_cast<float>(rect.x) + rect.width * 0.5f;
	const float centerY =
		static_cast<float>(rect.y) + rect.height * 0.5f;

	return
	{
		centerX * 2.0f / viewportSize_.x - 1.0f,
		1.0f - centerY * 2.0f / viewportSize_.y
	};
}

Vector2 UIManager::ToClipHalfSize(const UIPixelRect& rect) const
{
	return
	{
		static_cast<float>(rect.width) / viewportSize_.x,
		static_cast<float>(rect.height) / viewportSize_.y
	};
}

void UIManager::DrawProfilerPanel(Renderer& renderer) const
{
	const UIPixelRect panel
	{
		kProfilerX,
		kProfilerY,
		kProfilerWidth,
		kProfilerHeight
	};

	renderer.DrawUIRect(
		ToClipCenter(panel),
		ToClipHalfSize(panel),
		{ 0.035f, 0.045f, 0.07f, 0.94f });

	for (const ProfilerButton& button : kProfilerButtons)
	{
		renderer.DrawUIRect(
			ToClipCenter(button.bounds),
			ToClipHalfSize(button.bounds),
			button.color);
	}

	// 글자 텍스처 출력도 Renderer 경로를 사용한다.
	renderer.DrawUITexture(
		textTextureView_.Get(),
		ToClipCenter(panel),
		ToClipHalfSize(panel));
}
