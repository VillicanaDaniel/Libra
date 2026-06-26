#pragma once
#include "Game/Entity.hpp"

class Leo : public Entity
{
public:
	Leo(Game* owner, Vec2 const& pos);

	virtual void Update() override;
	virtual void Render() const override;
	virtual void setDead() override;

private:
	Vec2 m_targetPos = Vec2(0.f,0.f);
	float m_moveSpeed;
	float m_shootCooldown;
	float m_turnRate;
	float m_timeSinceLastShot;
	float m_roamCooldown;
	float m_goalOrientationDegrees;
	bool m_hasSpottedPlayer = false;
};