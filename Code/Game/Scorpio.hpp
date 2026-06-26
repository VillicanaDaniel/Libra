#pragma once
#include "Game/Entity.hpp"

class Scorpio : public Entity
{
public:
	Scorpio(Game* owner, Vec2 const& pos);

	virtual void Update() override;
	virtual void Render() const override;
	virtual void setDead() override;
	void RenderBase() const;
	void RenderTurret() const;
	void RenderLaser(Vec2 const& start, Vec2 const& end) const;

private:
	float m_turnRate;
	float m_fireCooldown;
	float m_timeSinceLastShot;
	bool m_hasSpottedPlayer = false;
};