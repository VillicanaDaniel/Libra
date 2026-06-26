#include "Engine/Core/Engine.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"
#include "Game/Player.hpp"
#include "Game/Scorpio.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/MathUtils.hpp"

Scorpio::Scorpio(Game* owner, Vec2 const& startPos)
	: Entity(owner, startPos, Vec2(0.f,0.f), 7)
{
	m_faction = FACTION_EVIL;
	m_isPushedByWalls = true;
	m_isPushedByEntities = false;
	m_doesPushEntities = true;
	m_isHitByBullets = true;

	m_game = owner;
	m_cosmeticRadius = g_gameConfigBlackboard.GetValue( "ScorpiocosmeticRadius", m_cosmeticRadius );
	m_physicsRadius = g_gameConfigBlackboard.GetValue( "ScorpiophysicsRadius", m_physicsRadius );
	m_turnRate = g_gameConfigBlackboard.GetValue( "ScorpioturnRate", m_turnRate );
	m_fireCooldown = g_gameConfigBlackboard.GetValue( "ScorpiofireCooldown", m_fireCooldown );
	m_timeSinceLastShot = g_gameConfigBlackboard.GetValue( "ScorpiotimeSinceLastShot", m_timeSinceLastShot);
}

void Scorpio::Update()
{
	float dt = (float)m_game->m_gameClock->GetDeltaSeconds();
	m_timeSinceLastShot += dt;

	Entity* player = m_game->m_currentMap->m_player;
	if (player == nullptr || player->m_isDead) return;

	Vec2 toPlayer = player->m_position - m_position;
	float dist = toPlayer.GetLength();
	float targetAngle = toPlayer.GetOrientationDegrees();
	bool canSeePlayer = (dist < 6.5f) && m_game->m_currentMap->HasLineOfSight(m_position, player->m_position);

	if (canSeePlayer)
	{
		if (!m_hasSpottedPlayer)
		{
			g_engine->m_audio->StartSound(m_game->m_spottedSound, false, 0.5f, 0.f, 1.f, false);
			m_hasSpottedPlayer = true;
		}
		m_orientationDegrees = GetTurnedTowardDegrees(m_orientationDegrees, targetAngle, m_turnRate * dt);

		float angleDiff = GetShortestAngularDispDegrees(m_orientationDegrees, targetAngle);
		if (fabsf(angleDiff) <= 5.f && m_timeSinceLastShot >= m_fireCooldown)
		{
			Vec2 spawnPos = m_position + GetForwardNormal() * 0.4f;
			m_game->m_currentMap->SpawnNewEntity(ENTITY_TYPE_EVIL_BOLT, spawnPos, m_orientationDegrees);
			m_timeSinceLastShot = 0.f;
		}
	}
	else
	{
		m_hasSpottedPlayer = false;
		m_orientationDegrees += m_turnRate * dt; 
	}

	if (m_health <= 0)
	{
		g_engine->m_audio->StartSound(m_game->m_deathSound, false, 0.25f, 0.0f, 1.0f, false);
		setDead();
	}
}

void Scorpio::Render() const
{	
	RenderBase();
	RenderTurret();
}


void Scorpio::RenderBase() const
{
	std::vector<Vertex> tankTextureVerts;
	AABB2 quad(-0.5f, -0.5f, 0.5f, 0.5f);
	AddVertsForAABB2(tankTextureVerts, quad, Rgba8(255, 255, 255, 255), Vec2(0.f, 0.f), Vec2(1.f,1.f));
	TransformVertexArrayXY3D((int)tankTextureVerts.size(), tankTextureVerts.data(), 1.f, 0.f, m_position);
	g_engine->m_render->BindTexture(m_game->m_scorpioBaseTexture);
	g_engine->m_render->DrawVertexArray(tankTextureVerts);
}


void Scorpio::RenderTurret() const
{
	Vec2 fwd = GetForwardNormal();
	Vec2 nosePos = m_position + fwd * 0.4f;

	RaycastResult2D ray = m_game->m_currentMap->RaycastVsTiles2D(nosePos, fwd, 6.f);

	Vec2 endPos = ray.m_didImpact ? ray.m_impactPos : (nosePos + fwd * 6.f);
	
	RenderLaser(nosePos, endPos);

	std::vector<Vertex> turretTextureVerts;
	AABB2 quad(-0.5f, -0.5f, 0.5f, 0.5f);
	AddVertsForAABB2(turretTextureVerts, quad, Rgba8(255, 255, 255, 255), Vec2(0.f, 0.f), Vec2(1.f,1.f));
	TransformVertexArrayXY3D((int)turretTextureVerts.size(), turretTextureVerts.data(), 1.f, m_orientationDegrees, m_position);
	g_engine->m_render->BindTexture(m_game->m_scorpioTurretTexture);
	g_engine->m_render->DrawVertexArray(turretTextureVerts);
}

void Scorpio::RenderLaser(Vec2 const& start, Vec2 const& end) const
{
	float thickness = 0.05f;

	Rgba8 nearColor(255, 0, 0, 255);
	Rgba8 farColor(80, 0, 0, 20);

	std::vector<Vertex> laserVerts;
	laserVerts.reserve(6);

	// Get perpendicular direction
	Vec2 fwdNorm = (end - start).GetNormalized();
	Vec2 left = fwdNorm.GetRotatedBy90Degrees();
	Vec2 offsetLeft = left * (thickness * 0.5f);
	Vec2 offsetRight = -offsetLeft;

	Vec2 p0 = start + offsetLeft;
	Vec2 p1 = start + offsetRight;
	Vec2 p2 = end + offsetRight;
	Vec2 p3 = end + offsetLeft;

	laserVerts.push_back(Vertex(p0, nearColor, Vec2(0.f,0.f)));
	laserVerts.push_back(Vertex(p1, nearColor, Vec2(0.f,0.f)));
	laserVerts.push_back(Vertex(p2, farColor, Vec2(0.f,0.f)));

	laserVerts.push_back(Vertex(p0, nearColor, Vec2(0.f,0.f)));
	laserVerts.push_back(Vertex(p2, farColor, Vec2(0.f,0.f)));
	laserVerts.push_back(Vertex(p3, farColor, Vec2(0.f,0.f)));

	g_engine->m_render->BindTexture(nullptr);
	g_engine->m_render->DrawVertexArray((int)laserVerts.size(), laserVerts.data());
}

void Scorpio::setDead()
{
	m_isDead = true;
	m_isGarbage = true;
	float explosionSize =g_gameConfigBlackboard.GetValue( "scorpioExplosionSize", 0.1f);
	m_game->m_currentMap->SpawnNewEntity(ENTITY_TYPE_EXPLOSION, m_position, explosionSize);
}


