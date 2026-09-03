#include "Character.h"

void Character::TakeDamage(int damage, float)
{
	hp_ -= damage;

	if (hp_ < 0) hp_ = 0;
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