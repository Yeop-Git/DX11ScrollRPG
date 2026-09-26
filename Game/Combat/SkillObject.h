#pragma once
#include "../Entity/GameObject.h"
#include "AttackDefinition.h"
#include <cstdint>
#include <vector>
#include <utility>

// 별도 스킬 루프에서 갱신한다. Entity 물리 루프에는 등록하지 않는다.
class SkillObject : public GameObject
{
public:
	void Spawn(AttackSlot slot, const AttackDefinition& definition, Vector2 position, bool right);
	RenderInfo GetRenderInfo() const override;
	bool RegisterContact(size_t monsterIndex, uint64_t serial);
	AttackSlot slot = AttackSlot::Q;
	const AttackDefinition* definition = nullptr;
	Vector2 previousPosition{};
	bool facingRight = true;
	double age = 0.0, previousAge = 0.0;
	unsigned nextTick = 0;

protected:
	void OnDisable() override;

private:
	std::vector<std::pair<size_t, uint64_t>> contacts_;
};
