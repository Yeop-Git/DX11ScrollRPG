#pragma once
// 접촉과 실제 피해, 최초 사망을 구분해 명중 효과와 보상을 중복 지급하지 않는다.
enum class DamageReaction
{
	NormalHit,
	Periodic
};
enum class DamageResult
{
	Ignored,
	Applied,
	Killed
};
struct DamageRequest
{
	int amount = 0;
	float attackerX = 0.0f;
	DamageReaction reaction = DamageReaction::NormalHit;
};
