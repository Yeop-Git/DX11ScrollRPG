#pragma once

#include "Entity/Character.h"
#include "Data/GameData.h"
#include "Stats/PlayerStat.h"
#include "Combat/PlayerAttack.h"
#include <memory>

enum class PlayerState
{
	Idle,
	Run,
	JumpStart,
	JumpEnd,
	Attack,
	SkillCast,
	Dead
};

class Player : public Character
{
public:
	// 생성자
	explicit Player(const GameData& data);
	~Player() override;

	void Update(float deltaTime) override;
	RenderInfo GetRenderInfo() const override;

	PlayerState GetState() const;

	DamageResult TakeDamage(const DamageRequest& request) override;

	void StartAttack();
	void FinishAttack();

	void Reset();
	void StartSkillCast();
	void GainExperience(uint64_t reward);
	const PlayerStat& GetStat() const { return stat_; }
	AttackAvailability GetAttackState() const;
	void SetCombatEnabled(bool enabled) { combatEnabled_ = enabled; }
	void SubmitAttack(int slot) { requestedAttack_ = slot; }
	void ConsumeAttack(GameWorld& world);
	std::array<AttackHudData, 5> GetAttackHudData() const;
	const AttackDefinition& GetNormalAttack() const { return attacks_[0]->GetDefinition(); }


	bool IsAttackFrameActive() const;
	bool CanRegisterAttackHit() const;
	void RegisterAttackHit();

	bool IsInvincible() const;
	bool ShouldRender() const;

	AABB GetAttackHitBox() const;

	const Animator& GetAnimator() const;

private:
	void HandleInput();
	void UpdateState(bool wasGrounded);
	void ChangeState(PlayerState newState);
	void UpdateDamageState(float deltaTime);

private:
	PlayerState state_ = PlayerState::Idle;

	// Application의 불변 설정을 빌린다. 설정은 이 객체보다 오래 살아야 한다.
	const PlayerDefinition& definition_;

	PlayerStat stat_;
	std::array<std::unique_ptr<PlayerAttack>, 5> attacks_;
	int requestedAttack_ = -1;
	bool combatEnabled_ = true;
	AttackAvailability lastFailure_ = AttackAvailability::Ready;
	int failedSlot_ = -1;
	float failureTimer_ = 0.0f;

	// Attack
	bool attackHitRegistered_ = false;

	// Hurt
	bool isInvincible_ = false;
	float invincibleTimer_ = 0.0f;
	float knockbackTimer_ = 0.0f;

};
