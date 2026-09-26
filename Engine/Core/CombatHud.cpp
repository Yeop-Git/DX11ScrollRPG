#include "CombatHud.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace
{
	constexpr int kIcon = 76, kGap = 12, kExperienceGap = 16;
	constexpr int kGrowthHeight = 32, kHeight = kIcon + kGrowthHeight;
	std::wstring Wide(const std::string& value)
	{
		if (value.empty())
			return {};
		const int count =
			MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
		std::wstring result(count, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), count);
		return result;
	}
} // namespace
bool CombatHud::Initialize(ID3D11Device* device, ID3D11DeviceContext* context, Vector2 viewport)
{
	context_ = context;
	viewport_ = viewport;
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = static_cast<UINT>(viewport_.x);
	desc.Height = kHeight;
	desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	if (FAILED(device->CreateTexture2D(&desc, nullptr, texture_.GetAddressOf())))
		return false;
	if (FAILED(device->CreateShaderResourceView(texture_.Get(), nullptr, view_.GetAddressOf())))
		return false;
	Update(data_);
	return !textCache_.empty();
}
void CombatHud::Update(const CombatHudData& data)
{
	data_ = data;
	std::array<std::wstring, 6> lines;
	std::wostringstream growth;
	growth << L"Lv." << data.level << L"   EXP ";
	if (data.maxLevel)
		growth << L"MAX";
	else
		growth << data.experience << L" / " << data.requiredExperience;
	if (data.dead)
		growth << L"     R: Restart";
	lines[0] = growth.str();
	for (size_t i = 0; i < data.attacks.size(); ++i)
	{
		const auto& attack = data.attacks[i];
		std::wostringstream text;
		text << Wide(attack.key) << L"\n";
		const bool locked = attack.availability == AttackAvailability::Locked;
		// 해금 레벨은 잠금 중에만 표시. 준비 상태에는 아이콘과 단축키만 남긴다.
		if (locked)
			text << L"Lv." << attack.requiredLevel;
		text << L"\n";
		if (!locked && attack.remaining > 0)
			text << std::fixed << std::setprecision(1) << std::ceil(attack.remaining * 10.0f) / 10.0f << L"s";
		lines[i + 1] = text.str();
	}
	std::wstring cache;
	for (const auto& line : lines)
		cache += line + L"|";
	// 상태 변경은 즉시 반영. 시간은 표시 문자열이 달라지는 0.1초 단위로만 업로드한다.
	if (cache != textCache_ && UploadText(lines))
		textCache_ = std::move(cache);
}
bool CombatHud::UploadText(const std::array<std::wstring, 6>& lines)
{
	const int kWidth = static_cast<int>(viewport_.x);
	HDC dc = CreateCompatibleDC(nullptr);
	if (!dc)
		return false;
	BITMAPINFO info{};
	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biWidth = kWidth;
	info.bmiHeader.biHeight = -kHeight;
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biCompression = BI_RGB;
	void* pixels = nullptr;
	HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
	HFONT font =
		CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
					CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Segoe UI");
	if (!bitmap || !font || !pixels)
	{
		if (bitmap)
			DeleteObject(bitmap);
		if (font)
			DeleteObject(font);
		DeleteDC(dc);
		return false;
	}
	auto oldBitmap = SelectObject(dc, bitmap);
	auto oldFont = SelectObject(dc, font);
	std::memset(pixels, 0, kWidth * kHeight * 4);
	SetBkMode(dc, TRANSPARENT);
	SetTextColor(dc, RGB(255, 255, 255));
	// 아이콘 내부 글자와 성장 글자를 한 텍스처에 저장하고 각각의 화면 위치로 옮긴다.
	RECT levelRect{16, kIcon + 4, kWidth - 16, kHeight};
	DrawTextW(dc, lines[0].c_str(), -1, &levelRect, DT_LEFT | DT_NOPREFIX);
	HFONT countdownFont =
		CreateFontW(-24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
					CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Segoe UI");
	for (int i = 0; i < 5; ++i)
	{
		const int x = SkillX(i);
		std::wistringstream text(lines[i + 1]);
		std::array<std::wstring, 3> parts;
		for (auto& part : parts)
			std::getline(text, part);
		auto draw = [&](const std::wstring& value, RECT rect) {
			DrawTextW(dc, value.c_str(), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		};
		draw(parts[0], {x + kIcon - 34, 0, x + kIcon, 20});
		draw(parts[1], {x, 58, x + kIcon, kIcon});
		if (countdownFont)
			SelectObject(dc, countdownFont);
		draw(parts[2], {x, 26, x + kIcon, 57});
		SelectObject(dc, font);
	}
	if (countdownFont)
		DeleteObject(countdownFont);
	GdiFlush();
	D3D11_MAPPED_SUBRESOURCE mapped{};
	ID3D11ShaderResourceView* empty = nullptr;
	context_->PSSetShaderResources(0, 1, &empty);
	const bool uploaded = SUCCEEDED(context_->Map(texture_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
	if (uploaded)
	{
		const auto* source = static_cast<const unsigned char*>(pixels);
		for (int y = 0; y < kHeight; ++y)
		{
			auto* target = static_cast<unsigned char*>(mapped.pData) + y * mapped.RowPitch;
			for (int x = 0; x < kWidth; ++x)
			{
				const auto* color = source + (y * kWidth + x) * 4;
				target[x * 4] = target[x * 4 + 1] = target[x * 4 + 2] = 255;
				target[x * 4 + 3] = (std::max)({color[0], color[1], color[2]});
			}
		}
		context_->Unmap(texture_.Get(), 0);
	}
	SelectObject(dc, oldFont);
	SelectObject(dc, oldBitmap);
	DeleteObject(font);
	DeleteObject(bitmap);
	DeleteDC(dc);
	return uploaded;
}
Vector2 CombatHud::Center(int x, int y, int width, int height) const
{
	return {(x + width * 0.5f) * 2.0f / viewport_.x - 1.0f, 1.0f - (y + height * 0.5f) * 2.0f / viewport_.y};
}
Vector2 CombatHud::HalfSize(int width, int height) const
{
	return {width / viewport_.x, height / viewport_.y};
}
int CombatHud::SkillX(int index) const
{
	// 오른쪽 여백 24px을 기준으로 Ctrl → Q → W → E → R 순서를 유지한다.
	return static_cast<int>(viewport_.x) - 24 - (5 * kIcon + 4 * kGap) + index * (kIcon + kGap);
}
int CombatHud::SkillY() const
{
	// EXP 패널 위에 간격을 두고 아이콘 행을 화면 하단에 고정한다.
	return static_cast<int>(viewport_.y) - 42 - kExperienceGap - kIcon;
}
void CombatHud::DrawLock(Renderer& renderer, int x, int y) const
{
	// 자물쇠는 해금 조건의 표시이며 게임 상태를 변경하지 않는다.
	const RendererColor gold{0.95f, 0.80f, 0.44f, 1};
	auto rect = [&](int dx, int dy, int w, int h, RendererColor color) {
		renderer.DrawUIRect(Center(x + dx, y + dy, w, h), HalfSize(w, h), color);
	};
	rect(28, 22, 20, 5, gold);
	rect(28, 27, 5, 12, gold);
	rect(43, 27, 5, 12, gold);
	rect(24, 36, 28, 22, gold);
	rect(36, 42, 4, 10, {0.10f, 0.12f, 0.17f, 1});
}
void CombatHud::DrawSkills(Renderer& renderer) const
{
	for (int i = 0; i < 5; ++i)
	{
		const auto& attack = data_.attacks[i];
		const int x = SkillX(i), y = SkillY();
		const bool locked = attack.availability == AttackAvailability::Locked;
		renderer.DrawUIRect(Center(x - 2, y - 2, kIcon + 4, kIcon + 4), HalfSize(kIcon + 4, kIcon + 4),
							locked ? RendererColor{0.30f, 0.32f, 0.38f, 1}
								   : RendererColor{0.55f, 0.68f, 0.72f, 1});
		// 투명 아이콘도 동일한 어두운 바탕 위에 표시한다.
		renderer.DrawUIRect(Center(x, y, kIcon, kIcon), HalfSize(kIcon, kIcon), {0.08f, 0.07f, 0.10f, 1});
		renderer.DrawUIIcon(attack.icon, Center(x, y, kIcon, kIcon), HalfSize(kIcon, kIcon));
		if (locked)
		{
			renderer.DrawUIRect(Center(x, y, kIcon, kIcon), HalfSize(kIcon, kIcon), {0, 0, 0, 0.72f});
			DrawLock(renderer, x, y);
		}
		else if (attack.remaining > 0 && attack.cooldown > 0)
		{
			// 실시간 비율은 매 프레임 그리되 글자 업로드는 0.1초 단위 캐시를 사용한다.
			renderer.DrawUICooldown(Center(x, y, kIcon, kIcon), HalfSize(kIcon, kIcon),
									attack.remaining / attack.cooldown, {0, 0, 0, 0.76f});
			// 중앙 숫자의 배경으로 밝은 아이콘 위에서도 대비를 확보한다.
			renderer.DrawUIRect(Center(x + 8, y + 27, kIcon - 16, 30), HalfSize(kIcon - 16, 30),
								{0, 0, 0, 0.45f});
		}
		else if (attack.availability != AttackAvailability::Ready)
			renderer.DrawUIRect(Center(x, y, kIcon, kIcon), HalfSize(kIcon, kIcon), {0, 0, 0, 0.40f});
		renderer.DrawUIRect(Center(x + kIcon - 34, y, 34, 20), HalfSize(34, 20),
							{0.02f, 0.03f, 0.05f, 0.92f});
	}
}
void CombatHud::DrawExperience(Renderer& renderer) const
{
	const int width = static_cast<int>(viewport_.x), bottom = static_cast<int>(viewport_.y);
	renderer.DrawUIRect(Center(0, bottom - 42, width, 42), HalfSize(width, 42),
						{0.035f, 0.045f, 0.07f, 0.94f});
	renderer.DrawUIRect(Center(0, bottom - 10, width, 10), HalfSize(width, 10), {0.12f, 0.15f, 0.20f, 1});
	const double ratio = data_.maxLevel ? 1.0
						 : data_.requiredExperience
							 ? static_cast<double>(data_.experience) / data_.requiredExperience
							 : 0.0;
	const int fill = static_cast<int>(width * std::clamp(ratio, 0.0, 1.0));
	if (fill > 0)
		renderer.DrawUIRect(Center(0, bottom - 10, fill, 10), HalfSize(fill, 10), {0.25f, 0.70f, 0.55f, 1});
}
void CombatHud::Render(Renderer& renderer) const
{
	DrawSkills(renderer);
	DrawExperience(renderer);
	const int width = static_cast<int>(viewport_.x), bottom = static_cast<int>(viewport_.y);
	const float split = static_cast<float>(kIcon) / kHeight;
	renderer.DrawUITexture(view_.Get(), Center(0, SkillY(), width, kIcon), HalfSize(width, kIcon), {},
						   {1, split});
	renderer.DrawUITexture(view_.Get(), Center(0, bottom - 42, width, kGrowthHeight),
						   HalfSize(width, kGrowthHeight), {0, split}, {1, 1});
}
