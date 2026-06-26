#include "Game/Explosion.hpp"
#include "Game/Game.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include <vector>

Explosion::Explosion(Game* owner, Vec2 const& position, float size)
	: Entity(owner, position, Vec2(0.f,0.f), 1)
{
	m_game = owner;
	m_size = size;

	m_isPushedByWalls = false;
	m_isPushedByEntities = false;
	m_doesPushEntities = false;
	m_isHitByBullets = false;
}

void Explosion::Update()
{
	float dt = (float)m_game->m_gameClock->GetDeltaSeconds();
	m_explosionElapsedTime += dt;
	m_currentExplosionFrame = &m_game->m_explosionAnim->GetSpriteDefAtTime(m_explosionElapsedTime);

	if (m_explosionElapsedTime >= 2.f)
	{
		m_isDead = true;
		m_isGarbage = true;
	}
}

void Explosion::Render() const
{
	{
		if (!m_currentExplosionFrame)
			return;

		std::vector<Vertex> verts;

		Vec2 uvMins, uvMaxs;
		m_currentExplosionFrame->GetUVs(uvMins, uvMaxs);

		float size = m_size; // bullets maybe use 1.0f, player death uses 5.0f, etc

		AABB2 bounds(
			m_position + Vec2(-size, -size),
			m_position + Vec2(+size, +size)
		);

		g_engine->m_render->SetBlendMode(BlendMode::ADDITIVE);

		AddVertsForAABB2(verts, bounds, Rgba8(255, 255, 255), uvMins, uvMaxs);

		g_engine->m_render->BindTexture(&m_currentExplosionFrame->GetTexture());
		g_engine->m_render->DrawVertexArray((int)verts.size(), verts.data());

		g_engine->m_render->SetBlendMode(BlendMode::ALPHA);
	}
}

