#include "Game/Bullet.hpp"
#include "Game/Game.hpp"
#include "Game/Map.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Texture.hpp"
#include <vector>

Bullet::Bullet(Game* owner, Vec2 const& position, Vec2 const& velocity, EntityFaction faction)
	: Entity(owner, position, Vec2(0.f,0.f), 1)
{
	m_game = owner; 
	m_velocity = velocity;
	m_faction = faction;

	m_physicsRadius = 0.001f;
	m_cosmeticRadius = 0.04f;
	m_doesPushEntities = false;
	m_isPushedByEntities = false;
	m_isPushedByWalls = false;
	m_isHitByBullets = false;

	if (velocity.GetLengthSquared() > 0.f)
	{
		m_orientationDegrees = Atan2Degrees(velocity.y, velocity.x);
	}

	if (faction == FACTION_GOOD)
		m_type = ENTITY_TYPE_GOOD_BULLET;
	else if (faction == FACTION_EVIL)
		m_type = ENTITY_TYPE_EVIL_BULLET;
}

void Bullet::Update()
{
	float dt = (float)m_game->m_gameClock->GetDeltaSeconds();
	m_position += m_velocity * dt;
}

void Bullet::Render() const
{
	std::vector<Vertex> verts;

	Texture* bulletTex = nullptr;

	switch (m_type)
	{
	case ENTITY_TYPE_GOOD_BULLET:
		bulletTex = m_game->m_goodBulletTexture;
		break;

	case ENTITY_TYPE_EVIL_BULLET:
		bulletTex = m_game->m_evilBulletTexture;
		break;

	case ENTITY_TYPE_EVIL_BOLT:
		bulletTex = m_game->m_evilBoltTexture;
		break;
	}

	if (bulletTex)
	{
		float aspect = (float)bulletTex->GetDimensions().x / (float)bulletTex->GetDimensions().y;

		float halfHeight = m_cosmeticRadius;
		float halfWidth = halfHeight * aspect;

		AABB2 quad = AABB2(
			Vec2(-halfWidth, -halfHeight),
			Vec2(halfWidth, halfHeight)
		);

		AddVertsForAABB2(verts, quad, Rgba8(255,255,255,255), Vec2(0.f, 0.f), Vec2(1.f, 1.f));
		TransformVertexArrayXY3D((int)verts.size(), verts.data(), 1.f, m_orientationDegrees, m_position);
		g_engine->m_render->BindTexture(bulletTex);
		g_engine->m_render->DrawVertexArray(verts);
	}
	else
	{
		AddVertsForDisc2D(verts, m_position, 0.1f, Rgba8(255, 255, 0, 255));
		g_engine->m_render->DrawVertexArray(verts);
	}
}

void Bullet::setDead()
{
		m_isDead = true;
		m_isGarbage = true;
		/*m_game->m_currentMap->SpawnNewEntity(ENTITY_TYPE_EXPLOSION, m_position, 0.25f);*/
}


