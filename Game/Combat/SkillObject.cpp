#include "SkillObject.h"
#include <algorithm>
#include <cmath>

void SkillObject::Spawn(AttackSlot newSlot, const AttackDefinition& data, Vector2 position, bool right)
{
	slot = newSlot;
	definition = &data;
	transform.position = position;
	previousPosition = position;
	facingRight = right;
	age = previousAge = 0.0;
	nextTick = 0;
	contacts_.clear();
	collider.enabled = true;
	collider.halfSize = data.hitBox;
	collider.offset = {};
}
bool SkillObject::RegisterContact(size_t index, uint64_t serial)
{
	const auto key = std::make_pair(index, serial);
	if (std::find(contacts_.begin(), contacts_.end(), key) != contacts_.end()) return false;
	contacts_.push_back(key);
	return true;
}
RenderInfo SkillObject::GetRenderInfo() const
{
	RenderInfo info;
	if (!definition || definition->frames.empty())
	{
		info.visible = false;
		return info;
	}
	size_t frame = static_cast<size_t>(age / definition->frameDuration);
	frame = definition->loop ? frame % definition->frames.size()
							 : (std::min)(frame, definition->frames.size() - 1);
	info.spriteId = definition->frames[frame];
	info.position = transform.position;
	info.frameSizePixels = {64.0f, 64.0f};
	info.renderHalfSize = definition->renderHalfSize;
	info.renderMode = SpriteRenderMode::AlphaBlend;
	info.flipX = !facingRight;
	return info;
}

void SkillObject::OnDisable()
{
	// 반환 직후부터 충돌/시각 정의와 이전 회차의 명중 이력을 제거한다.
	definition = nullptr;
	collider.enabled = false;
	contacts_.clear();
	age = previousAge = 0.0;
	nextTick = 0;
	transform.position = previousPosition = {};
}
