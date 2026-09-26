#pragma once
#include "../Entity/GameObject.h"
#include "../Combat/AttackDefinition.h"

class HitEffect : public GameObject
{
public:
	void Spawn(const EffectDefinition& definition, Vector2 position);
	bool Advance(float dt);
	RenderInfo GetRenderInfo() const override;

protected:
	void OnDisable() override;

private:
	const EffectDefinition* definition_ = nullptr;
	double age_ = 0.0;
};
