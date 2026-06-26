#pragma once
#include "Engine/Math/Vec2.hpp"
#include <vector>

class Game;

enum EntityType
{
	ENTITY_TYPE_GOOD_PLAYER,
	ENTITY_TYPE_EVIL_SCORPIO,
	ENTITY_TYPE_EVIL_LEO,
	ENTITY_TYPE_EVIL_ARIES,
	ENTITY_TYPE_GOOD_BULLET,
	ENTITY_TYPE_EVIL_BULLET,
	ENTITY_TYPE_EVIL_BOLT,
	ENTITY_TYPE_EXPLOSION,
	NUM_ENTITY_TYPES
};

enum EntityFaction
{
	FACTION_GOOD = 0,
	FACTION_NEUTRAL,
	FACTION_EVIL
};

typedef std::vector<class Entity*> EntityList;

class Entity
{
public:
	Game* m_game = nullptr;
	Vec2 m_position;
	Vec2 m_velocity;
	float m_orientationDegrees;
	float m_angularVelocity;
	float m_physicsRadius;
	float m_cosmeticRadius;
	bool m_isDead = false;
	bool m_isGarbage = false;
	int m_health = 1;

	EntityFaction m_faction = FACTION_NEUTRAL;
	EntityType m_type = ENTITY_TYPE_GOOD_PLAYER;

	bool m_isPushedByEntities = true;
	bool m_doesPushEntities = true;
	bool m_isPushedByWalls = true;
	bool m_isHitByBullets = true;

public:
	Entity( Game* owner, Vec2 const& startingPosition, Vec2 const& startingVelocity, int health = 1 );
	virtual ~Entity() = default;

	virtual void Update() = 0;
	virtual void Render() const = 0;

	virtual void RenderDebug() const;
	virtual void TakeDamage(int damage);
	virtual void setDead();

	Vec2 GetForwardNormal() const;
};