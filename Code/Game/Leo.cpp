#include "Engine/Core/Engine.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"
#include "Game/Player.hpp"
#include "Game/Leo.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

Leo::Leo(Game* owner, Vec2 const& pos)
	: Entity(owner, pos, Vec2(0.f,0.f), 5)
{
	m_faction = FACTION_EVIL;
	m_isPushedByWalls = true;
	m_isPushedByEntities = true;
	m_doesPushEntities = true;
	m_isHitByBullets = true;

	m_game = owner;
	m_cosmeticRadius = g_gameConfigBlackboard.GetValue( "LeocosmeticRadius", m_cosmeticRadius );
	m_physicsRadius = g_gameConfigBlackboard.GetValue( "LeophysicsRadius", m_physicsRadius );

	m_moveSpeed = g_gameConfigBlackboard.GetValue( "LeomoveSpeed", m_moveSpeed );
	m_shootCooldown = g_gameConfigBlackboard.GetValue( "LeoshootCooldown", m_shootCooldown  );
	m_turnRate = g_gameConfigBlackboard.GetValue( "LeoturnRate", m_turnRate  );
	m_timeSinceLastShot = g_gameConfigBlackboard.GetValue( "LeotimeSinceLastShot", m_timeSinceLastShot );
	m_roamCooldown = g_gameConfigBlackboard.GetValue( "LeoroamCooldown", m_roamCooldown );
	m_goalOrientationDegrees = g_gameConfigBlackboard.GetValue( "LeogoalOrientationDegrees", m_goalOrientationDegrees );
}

void Leo::Update()
{
	float dt = (float)m_game->m_gameClock->GetDeltaSeconds();
	m_timeSinceLastShot += dt;
	Entity* player = m_game->m_currentMap->m_player;
	if (!player) return;

	Vec2 toPlayer = player->m_position - m_position;
	float dist = toPlayer.GetLength();
	bool canSeePlayer = (dist < 7.0f) && m_game->m_currentMap->HasLineOfSight(m_position, player->m_position);

	if (canSeePlayer)
	{
		//Play spotted sound, flag that the player has been spotted
		if (!m_hasSpottedPlayer)
		{
			g_engine->m_audio->StartSound(m_game->m_spottedSound, false, 0.5f, 0.f, 1.f, false);
			m_hasSpottedPlayer = true;
		}
		
		//Then perform the player spotted behavior
		m_targetPos = player->m_position;
		Vec2 toTarget = m_targetPos - m_position;
		m_goalOrientationDegrees = toTarget.GetOrientationDegrees();
		m_orientationDegrees = GetTurnedTowardDegrees(m_orientationDegrees, m_goalOrientationDegrees, m_turnRate * dt);

		float angleDiffToPlayer = GetShortestAngularDispDegrees(m_orientationDegrees, m_goalOrientationDegrees);
		if (fabsf(angleDiffToPlayer) <= 45.f)
		{
			m_velocity = GetForwardNormal() * m_moveSpeed;
			m_position += m_velocity * dt;
		}

		if (fabsf(angleDiffToPlayer) <= 5.f && m_timeSinceLastShot >= m_shootCooldown)
		{
			Vec2 spawnPos = m_position + GetForwardNormal() * m_moveSpeed;
			m_game->m_currentMap->SpawnNewEntity(ENTITY_TYPE_EVIL_BULLET, spawnPos, m_orientationDegrees);
			m_timeSinceLastShot = 0.f;
		}
	}
	else
	{
		if (m_roamCooldown <= 0.f)
		{
			m_goalOrientationDegrees = g_rng->RollRandomFloatInRange(0.f, 360.f);
			m_roamCooldown = g_rng->RollRandomFloatInRange(1.f, 3.f);
		}
		m_orientationDegrees = GetTurnedTowardDegrees(m_orientationDegrees, m_goalOrientationDegrees, m_turnRate * dt);
		m_velocity = GetForwardNormal() * 0.5f;
		m_position += m_velocity * dt;
		m_roamCooldown -= dt;

		m_hasSpottedPlayer = false;
	}

	if (m_health <= 0)
	{
		g_engine->m_audio->StartSound(m_game->m_deathSound, false, 0.25f, 0.0f, 1.0f, false);
		setDead();
	}
}

void Leo::Render() const
{
	std::vector<Vertex> tankTextureVerts;
	AABB2 quad(-0.5f, -0.5f, 0.5f, 0.5f);
	AddVertsForAABB2(tankTextureVerts, quad, Rgba8(255, 255, 255, 255), Vec2(0.f, 0.f), Vec2(1.f,1.f));
	TransformVertexArrayXY3D((int)tankTextureVerts.size(), tankTextureVerts.data(), 1.f, m_orientationDegrees, m_position);
	g_engine->m_render->BindTexture(m_game->m_leoTexture);
	g_engine->m_render->DrawVertexArray(tankTextureVerts);
}

void Leo::setDead()
{
	m_isDead = true;
	m_isGarbage = true;
	float explosionSize = g_gameConfigBlackboard.GetValue("tankExplosionSize", 0.1f);
	m_game->m_currentMap->SpawnNewEntity(ENTITY_TYPE_EXPLOSION, m_position, explosionSize);
}