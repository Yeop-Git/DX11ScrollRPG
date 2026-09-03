#pragma once

#include "Transform.h"
#include "Collider.h"
#include "../../Engine/Graphics/RenderInfo.h"

// 월드에서 Update되는 모든 객체를 Entity로 추상화
class Entity
{
public:
	Transform transform;
	Collider collider;

	// 소멸자 virtual
	virtual ~Entity() = default;

	// Update를 순수 virtual 함수로 하여 자식에서 구현을 강제
	virtual void Update(float deltaTime) = 0;
	virtual RenderInfo GetRenderInfo() const = 0;

	AABB GetBodyBox() const
	{
		return collider.GetBounds(transform);
	}

	bool IsActive() const { return active_; }

	void SetActive(bool active) { active_ = active; }

	bool active_ = true;
};