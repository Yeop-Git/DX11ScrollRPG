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
	transform.x += velocityX_ * deltaTime;
	transform.y += velocityY_ * deltaTime;

	// 화면 밖으로 나가지 않도록 clamp
	transform.x = std::clamp(transform.x, -1.0f + collider.halfHeight, 1.0f - collider.halfWidth);
}

RenderInfo Character::GetRenderInfo() const
{
	const AnimationClip& clip =
		animator_.GetCurrentClip();

	RenderInfo info;

	info.x = transform.x;
	info.y = transform.y;

	info.frame = animator_.GetCurrentFrame();
	info.frameCount = animator_.GetFrameCount();

	info.frameWidthPx = clip.frameWidthPx;
	info.frameHeightPx = clip.frameHeightPx;

	info.renderHalfHeight =clip.renderHalfHeight;

	info.offsetX = clip.offsetX;
	info.offsetY = clip.offsetY + renderOffsetY;

	return info;
}