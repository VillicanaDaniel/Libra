#include "Game/Entity.hpp"
#include "Game/Game.hpp"
#include "Game/Map.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vec2.hpp"

Entity::Entity(Game* owner, Vec2 const& startingPosition, Vec2 const& startingVelocity, int health)
	:m_game( owner)
	,m_position( startingPosition )
	,m_velocity( startingVelocity )
	,m_health( health )
{
}


void Entity::RenderDebug() const
{	
	g_engine->m_render->BindTexture(nullptr);

	//Draw Rings
	DebugDrawRing(m_position, m_cosmeticRadius, 0.025f, Rgba8(255, 0, 255));
	DebugDrawRing(m_position, m_physicsRadius, 0.025f, Rgba8(0, 255, 255));

	//Draw forward and perpendicular line to cosmetic radius
	DebugDrawLine(m_position, m_position + GetForwardNormal() * m_cosmeticRadius, 0.05f, Rgba8(255, 0, 0));
	DebugDrawLine(m_position, m_position + (GetForwardNormal().GetRotatedBy90Degrees()) * m_cosmeticRadius, 0.05f, Rgba8(0, 255, 0));

	//Draw Velocity Line
	DebugDrawLine(m_position, m_position + m_velocity, 0.05f, Rgba8(255, 255, 0));
}

Vec2 Entity::GetForwardNormal() const
{
	return Vec2(CosDegrees(m_orientationDegrees), SinDegrees(m_orientationDegrees));
}

void Entity::TakeDamage(int damage)
{
	m_health -= damage;
}

void Entity::setDead() 
{
	m_isDead = true;
	m_isGarbage = true;
}

