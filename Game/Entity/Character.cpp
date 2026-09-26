#include "Character.h"
#include <algorithm>

DamageResult Character::TakeDamage(const DamageRequest& request)
{
	if (IsDead() || request.amount <= 0) return DamageResult::Ignored;
	hp_ = (std::max)(0, hp_ - request.amount);
	return IsDead() ? DamageResult::Killed : DamageResult::Applied;
}

void Character::Heal(int amount)
{
	if (IsDead()) return;

	hp_ = std::min(hp_ + amount, maxHp_);
}

void Character::ClampWorld()
{
	// 화면 밖으로 나가지 않도록 clamp
	transform.position.x = std::clamp(
		transform.position.x,
		-1.0f + collider.halfSize.x,
		1.0f - collider.halfSize.x);
}

RenderInfo Character::GetRenderInfo() const
{
	const AnimationClip& clip =
		animator_.GetCurrentClip();

	RenderInfo info;

	info.position = transform.position;

	info.frame = animator_.GetCurrentFrame();
	info.frameCount = animator_.GetFrameCount();

	info.frameSizePixels = clip.frameSizePixels;

	info.renderHalfSize = clip.renderHalfSize;

	info.offset = clip.offset;
	info.offset.y += renderOffsetY;

	return info;
}
