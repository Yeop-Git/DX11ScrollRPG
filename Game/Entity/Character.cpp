#include "Character.h"
#include <algorithm>

void Character::TakeDamage(int damage, float)
{
	hp_ -= damage;

	if (hp_ < 0) hp_ = 0;
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
