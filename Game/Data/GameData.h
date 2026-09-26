#pragma once

#include <filesystem>
#include "../Stats/StatDefinitions.h"
#include "../Combat/AttackDefinition.h"
#include <string>
#include <unordered_map>

#include "../Animation/AnimationClip.h"
#include "../../Engine/Graphics/SpriteId.h"

// 디스크에서 읽는 정적 정의다. HP, 재생 시간, FSM 같은 실행 상태는 포함하지 않는다.
struct PlayerDefinition
{
	AnimationClip idle, run, jumpStart, jumpEnd, attack, dead;
	Vector2 colliderHalfSize{}, startPosition{}, knockbackSpeed{};
	float renderOffsetY = 0.0f;
	float moveSpeed = 0.0f;
	float jumpSpeed = 0.0f;
	float invincibleDuration = 0.0f;
	float knockbackDuration = 0.0f;
	float blinkInterval = 0.0f;
};

struct MonsterDefinition
{
	AnimationClip idle, chase, hurt, dead;
	Vector2 colliderHalfSize{}, startPosition{};
	float renderOffsetY = 0.0f;
	float chaseSpeed = 0.0f;
	float chaseRange = 0.0f;
	float knockbackSpeed = 0.0f;
};

struct GameData
{
	PlayerStatDefinition playerStat;
	EnemyStatDefinition enemyStat;
	AttackDefinitions attacks;
	EffectDefinition hitEffect;
	unsigned skillPoolSize = 64, effectPoolSize = 128;
	PlayerDefinition player;
	MonsterDefinition monster;
	std::unordered_map<SpriteId, std::string> textures;
};

// Worker에서 호출한다. 게임 객체/D3D에 접근하지 않고, 실패 시 파일·필드 정보를 담아 예외를 전달한다.
GameData LoadGameData(const std::filesystem::path& filePath);
