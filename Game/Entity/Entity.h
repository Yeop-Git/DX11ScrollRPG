#pragma once

#include "GameObject.h"
#include "Physics.h"

// 월드에서 Update되는 모든 객체를 Entity로 추상화
class Entity : public GameObject
{
public:
	// 소멸자 virtual
	virtual ~Entity() = default;

	// Update를 순수 virtual 함수로 하여 자식에서 구현을 강제
	virtual void Update(float deltaTime) = 0;

	Physics physics;
};