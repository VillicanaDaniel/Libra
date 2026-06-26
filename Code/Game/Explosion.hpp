#pragma once
#include "Game/Entity.hpp"
#include "Engine/Renderer/Texture.hpp"

class Explosion : public Entity
{
public:
	Explosion(Game* owner, Vec2 const& position, float size);
	virtual void Update() override;
	virtual void Render() const override;

public:
	int   m_currentFrame = 0;
	SpriteDef const* m_currentExplosionFrame = nullptr;
	float m_explosionElapsedTime = 0.f;
	float m_size;
};