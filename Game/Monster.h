#pragma once

#include "Entity/Character.h"
#include "Data/GameData.h"

enum class MonsterState
{
	Idle,
	Chase,
	Hurt,
	Dead
};

class Monster : public Character
{
public :
	explicit Monster(const MonsterDefinition& definition);

	// Update에서 playerX 제거
	void Update(float deltaTime) override;
	void OnEnable() override;
	void OnDisable() override;

	RenderInfo GetRenderInfo() const override;

	void TakeDamage(int damage, float attackerX);

	// Player를 Target, Character 포인터로 받음
	void SetTarget(Character* target);

	void FinishHit();
	void Reset();

	MonsterState GetState() const;
	const Animator& GetAnimator() const;

	void ChangeState(MonsterState newState);

	bool IsDeadAnimationFinished() const;

private:
	void UpdateState();

private :
	// Chase Target, 주로 플레이어
	Character* target_ = nullptr;
	MonsterState state_ = MonsterState::Idle;

	// Application의 불변 설정을 빌린다. 설정은 이 객체보다 오래 살아야 한다.
	const MonsterDefinition& definition_;

};
