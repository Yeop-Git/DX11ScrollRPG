#pragma once

#include "../../Engine/Graphics/RenderInfo.h"

// 월드에서 Update되는 모든 객체를 Entity로 추상화
class Entity
{
public :

	// 소멸자 virtual
	virtual ~Entity() = default;

	// Update를 순수 virtual 함수로 하여 자식에서 구현을 강제
	virtual void Update(float deltaTime) = 0;
	virtual RenderInfo GetRenderInfo() const = 0;

	float GetX() const
	{
		return x_;
	}

	float GetY() const
	{
		return y_;
	}

	bool IsActive() const
	{
		return active_;
	}

	void SetActive(bool active)
	{
		active_ = active;
	}

protected:
	// 월드 내 좌표와 활성화 여부만을 가짐
	float x_ = 0.0f;
	float y_ = 0.0f;

	bool active_ = true;
};