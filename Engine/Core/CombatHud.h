#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include "../Graphics/Renderer.h"
#include "../../Game/Combat/AttackDefinition.h"

// UIManager가 소유하는 표시 전용 캐시. 게임 객체나 성장 규칙은 소유하지 않는다.
struct CombatHudData
{
	unsigned level = 1;
	uint64_t experience = 0, requiredExperience = 0;
	bool maxLevel = false, dead = false;
	std::array<AttackHudData, 5> attacks;
};
class CombatHud
{
public:
	bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context, Vector2 viewport);
	void Update(const CombatHudData& data);
	void Render(Renderer& renderer) const;

private:
	int SkillX(int index) const;
	int SkillY() const;
	void DrawLock(Renderer& renderer, int x, int y) const;
	void DrawSkills(Renderer& renderer) const;
	void DrawExperience(Renderer& renderer) const;
	bool UploadText(const std::array<std::wstring, 6>& lines);
	Vector2 Center(int x, int y, int width, int height) const;
	Vector2 HalfSize(int width, int height) const;
	ID3D11DeviceContext* context_ = nullptr;
	Vector2 viewport_{};
	Microsoft::WRL::ComPtr<ID3D11Texture2D> texture_;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view_;
	CombatHudData data_;
	std::wstring textCache_;
};
