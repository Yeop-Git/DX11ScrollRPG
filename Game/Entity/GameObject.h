#pragma once

#include "Transform.h"
#include "Collider.h"
#include "../../Engine/Graphics/RenderInfo.h"

// 모든 게임오브젝트를 추상화, Transform과 Collider 소유
class GameObject
{
public:
	Transform transform;
	Collider collider;

public:
	GameObject() = default;

	// GameObject 정보 받고 생성 (클래스 필요없이)
	GameObject(SpriteId spriteId, Vector2 position, Vector2 halfSize);

	// 소멸자 virtual
	virtual ~GameObject() = default;

	// Update를 순수 virtual 함수로 하여 자식에서 구현을 강제
	virtual RenderInfo GetRenderInfo() const;

	// AABB Box Collider
	AABB GetBodyBox() const
	{
		return collider.GetBounds(transform);
	}

	// Active 관리
	bool IsActive() const { return active_; }

	void SetActive(bool active) 
	{ 
		if (active_ == active) return;
		active_ = active; 

		if (active_) OnEnable();
		else OnDisable();
	}

	// OnEnable, OnDisable 추가
protected:
	virtual void OnEnable(){}
	virtual void OnDisable() {}

private:
	bool active_ = true;

	SpriteId spriteId_= SpriteId::None;
	Vector2 renderSize_ = { 0.0f, 0.0f };
};