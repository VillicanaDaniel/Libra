#pragma once
#include "Game/Entity.hpp"

class Aries : public Entity
{
public:
	Aries(Game* owner, Vec2 const& pos);

	virtual void Update() override;
	virtual void Render() const override;
	virtual void setDead() override;

private:
	float m_turnRate;
	Vec2 m_targetPos = Vec2(0.f,0.f);
	float m_roamCooldown = 0.f;
	float m_goalOrientationDegrees = 0.f;
	bool m_hasSpottedPlayer = false;
};