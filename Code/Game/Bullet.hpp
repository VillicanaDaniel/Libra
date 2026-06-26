#pragma once
#include "Game/Entity.hpp"
#include "Engine/Core/Rgba8.hpp"

class Bullet : public Entity
{
public:
	Bullet(Game* owner, Vec2 const& position, Vec2 const& velocity, EntityFaction faction);
	virtual void Update() override;
	virtual void Render() const override;
	virtual void setDead() override;

public:
	int   m_bouncesRemaining = 2;
};