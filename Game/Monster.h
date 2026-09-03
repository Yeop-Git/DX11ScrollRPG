#pragma once

#include "Entity/Character.h"
#include "Animation/Animator.h"

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
	Monster();

	// Update에서 playerX 제거
	void Update(float deltaTime) override;

	void TakeDamage(int damage, float attackerX);

	// Player를 Target, Character 포인터로 받음
	void SetTarget(Character* target);

	void FinishHit();
	void Reset();

	MonsterState GetState() const;
	const Animator& GetAnimator() const;

	void ChangeState(MonsterState newState);

	AABB GetHurtBox() const;

private:
	void UpdateState(float playerX);

private :
	// Chase Target, 주로 플레이어
	Character* target_ = nullptr;
	MonsterState state_ = MonsterState::Idle;

	// Animation
	Animator animator_;
	
	AnimationClip idleClip_;
	AnimationClip chaseClip_;
	AnimationClip hurtClip_;
	AnimationClip deadClip_;

	// Chase
	static constexpr float kChaseSpeed = 0.25f;
	static constexpr float kChaseRange = 0.6f;

	// Hurt
	static constexpr float kKnockbackSpeed = 0.45f;

	// Animation Clip
	static constexpr int kIdleClipCount = 4;
	static constexpr int kChaseClipCount = 8;
	static constexpr int kHurtClipCount = 8;
	static constexpr int kDeadClipCount = 8;
};