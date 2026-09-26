#pragma once
#include <array>
#include <string>
#include <vector>
#include "../../Engine/Graphics/SpriteId.h"
#include "../../Engine/Math/Vector2.h"

enum class AttackSlot
{
	Normal,
	Q,
	W,
	E,
	R,
	Count
};
enum class AttackAvailability
{
	Ready,
	Locked,
	Dead,
	Hurt,
	Airborne,
	Casting,
	Cooldown,
	Disabled,
	PoolFull,
	NoGround
};
struct AttackDefinition
{
	std::string name, inputKey;
	SpriteId icon = SpriteId::None;
	int virtualKey = 0;
	unsigned requiredLevel = 1;
	float cooldown = 0.0f;
	int damage = 1;
	float speed = 0.0f, lifetime = 0.0f, hitInterval = 0.5f;
	Vector2 hitBox{}, spawnOffset{}, renderHalfSize{};
	float frameDuration = 0.08f;
	int firstActiveFrame = 0, lastActiveFrame = 0;
	// 개별 원본 PNG를 프레임으로 참조. 비행/소멸 프레임을 한 루프로 섞지 않는다.
	std::vector<SpriteId> frames;
	bool loop = true;
};
struct EffectDefinition
{
	std::vector<SpriteId> frames;
	float frameDuration = 0.06f;
	Vector2 renderHalfSize{0.08f, 0.14f};
};
struct AttackHudData
{
	std::string name, key;
	unsigned requiredLevel = 1;
	float cooldown = 0.0f, remaining = 0.0f;
	AttackAvailability availability = AttackAvailability::Ready;
	SpriteId icon = SpriteId::None;
};
using AttackDefinitions = std::array<AttackDefinition, 5>;
