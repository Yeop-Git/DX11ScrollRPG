#include "Character.h"
#include <algorithm>

void Character::TakeDamage(int damage, float)
{
	hp_ -= damage;

	if (hp_ < 0) hp_ = 0;
}

void Character::UpdatePosition(float deltaTime)
{
	// Velocity에 따른 위치 업데이트
	x_ += velocityX_ * deltaTime;
	y_ += velocityY_ * deltaTime;

	// 화면 밖으로 나가지 않도록 clamp
	x_ = std::clamp(x_, -1.0f + colliderHalfWidth_, 1.0f - colliderHalfWidth_);
}

AABB Character::GetBodyBox() const
{
	return
	{
		x_ - colliderHalfWidth_,
		x_ + colliderHalfWidth_,
		y_ - colliderHalfHeight_,
		y_ + colliderHalfHeight_
	};
}