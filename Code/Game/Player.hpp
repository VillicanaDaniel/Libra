#pragma once
#include "Game/Entity.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/Rgba8.hpp"

class Texture;

class Player : public Entity
{
public:
	Player(Game* owner, Vec2 const& startPos);

	virtual void Update() override;
	virtual void Render() const override;
	virtual void RenderDebug() const;
	virtual void setDead() override;
	virtual void TakeDamage(int damage) override;

private:
	void Move();
	void RotateTurret();
	void HandleShooting();
	void RenderTank() const;
	void RenderTurret() const;
	void RenderStatus() const;

	float m_turretOrientationDegrees;
	float m_goalTurretOrientation;
	float m_turretRelativeOrientation;
	float m_tankTurnSpeed;
	float m_turretTurnSpeed;
	float m_maxSpeed;
	float m_goalOrientation;

	float m_fireCooldown;
	float m_timeSinceLastShot;
	bool m_isFiring;
	
	Texture* m_tankTexture;
	Texture* m_turretTexture;

	void CreateorGetTextures();
};