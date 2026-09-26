#include "HitEffect.h"
#include <algorithm>
void HitEffect::Spawn(const EffectDefinition& definition, Vector2 position)
{
	definition_ = &definition;
	transform.position = position;
	age_ = 0.0;
	collider.enabled = false;
}
bool HitEffect::Advance(float dt)
{
	age_ += dt;
	return age_ >= definition_->frameDuration * definition_->frames.size();
}
RenderInfo HitEffect::GetRenderInfo() const
{
	RenderInfo info;
	if (!definition_)
	{
		info.visible = false;
		return info;
	}
	const auto frame =
		(std::min)(static_cast<size_t>(age_ / definition_->frameDuration), definition_->frames.size() - 1);
	info.spriteId = definition_->frames[frame];
	info.position = transform.position;
	info.frameSizePixels = {64.0f, 64.0f};
	info.renderHalfSize = definition_->renderHalfSize;
	info.renderMode = SpriteRenderMode::AlphaBlend;
	return info;
}

void HitEffect::OnDisable()
{
	definition_ = nullptr;
	age_ = 0.0;
	transform.position = {};
	collider.enabled = false;
}
