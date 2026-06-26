#include "Game/Player.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/Core/VertexUtils.hpp"

Player::Player(Game* owner, Vec2 const& startPos)
	: Entity(owner, startPos, Vec2(0.f,0.f), 10)
{
	m_game = owner;
	m_physicsRadius = g_gameConfigBlackboard.GetValue( "playerphysicsRadius", 0.3f );
	m_cosmeticRadius = g_gameConfigBlackboard.GetValue( "playercosmeticRadius", 0.5f );
	m_orientationDegrees = 0.f;
	m_turretOrientationDegrees = g_gameConfigBlackboard.GetValue( "playerTurretOrientation", m_turretOrientationDegrees );
	m_tankTurnSpeed = g_gameConfigBlackboard.GetValue( "playertankTurnSpeed", m_tankTurnSpeed);
	m_turretTurnSpeed = g_gameConfigBlackboard.GetValue( "playerturretTurnSpeed", m_turretTurnSpeed);
	m_maxSpeed = g_gameConfigBlackboard.GetValue( "playermaxSpeed", m_maxSpeed );
	m_fireCooldown = g_gameConfigBlackboard.GetValue( "playerfireCooldown", m_fireCooldown );
	m_timeSinceLastShot = g_gameConfigBlackboard.GetValue( "playertimeSinceLastShot", m_timeSinceLastShot );
	m_isFiring = g_gameConfigBlackboard.GetValue( "playerisFiring", m_isFiring);

	CreateorGetTextures();
}


void Player::Update()
{
	float dt = (float)m_game->m_gameClock->GetDeltaSeconds();
	if(m_isDead)
		return;
	Move();
	RotateTurret();

	m_timeSinceLastShot += dt;
	HandleShooting();

	if (m_health <= 0)
	{
		setDead();
	}
}


void Player::Render() const
{
	if(m_isDead)
		return;
	RenderTank();
	RenderTurret();
	RenderStatus();
}


void Player::RenderDebug() const
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

	//Draw Goal Move orientation
	Vec2 goalMoveDir = Vec2::MakeFromPolarDegrees(m_goalOrientation, m_cosmeticRadius);
	Vec2 goalMoveDirExt = Vec2::MakeFromPolarDegrees(m_goalOrientation, m_cosmeticRadius + 0.2f);
	DebugDrawLine(m_position + goalMoveDir, m_position + goalMoveDirExt, 0.05f, Rgba8(255, 0, 0));

	//Draw Turret orientation
	Vec2 turretDir = Vec2::MakeFromPolarDegrees(m_turretOrientationDegrees, m_cosmeticRadius);
	DebugDrawLine(m_position, m_position + turretDir, 0.075f, Rgba8(0, 0, 255));

	//Draw Goal Turret orientation
	Vec2 goalTurretDir = Vec2::MakeFromPolarDegrees(m_goalTurretOrientation, m_cosmeticRadius);
	Vec2 goalTurretDirExt = Vec2::MakeFromPolarDegrees(m_goalTurretOrientation, m_cosmeticRadius + 0.2f);
	DebugDrawLine(m_position + goalTurretDir, m_position + goalTurretDirExt, 0.05f, Rgba8(0, 0, 255));
}


void Player::setDead()
{
	g_engine->m_audio->StartSound(m_game->m_deathSound, false, 0.25f, 0.0f, 1.0f, false);
	m_isDead = true;
	float explosionSize = g_gameConfigBlackboard.GetValue("tankExplosionSize", 0.1f);
	m_game->m_currentMap->SpawnNewEntity(ENTITY_TYPE_EXPLOSION, m_position, explosionSize);
}

void Player::TakeDamage(int damage)
{
	if(m_game->m_isInvincible)
		return;
	m_health -= damage;
}

void Player::Move()
{
	float dt = (float)m_game->m_gameClock->GetDeltaSeconds();
	InputSystem* g_theInput = g_engine->m_input;
	XboxController const& controller = g_engine->m_input->GetController(0);
	Vec2 moveDir = Vec2(0.f, 0.f);

	//Keyboard(WASD)
	if (g_theInput->IsKeyDown('W'))
		moveDir.y += 1.f;
	if (g_theInput->IsKeyDown('S'))
		moveDir.y -= 1.f;
	if (g_theInput->IsKeyDown('A'))
		moveDir.x -= 1.f;
	if (g_theInput->IsKeyDown('D'))
		moveDir.x += 1.f;

	//Controller(left stick)
	Vec2 leftStickDir = controller.GetLeftStick().GetPosition();
	if (leftStickDir.GetLengthSquared() > 0.f)
	{
		moveDir = leftStickDir;
	}

	//Move Calculation
	if(moveDir.GetLengthSquared() > 0.f)
	{ 
		m_goalOrientation = moveDir.GetOrientationDegrees();
		m_orientationDegrees = GetTurnedTowardDegrees(m_orientationDegrees, m_goalOrientation, m_tankTurnSpeed * dt);
		m_velocity = GetForwardNormal() * m_maxSpeed;
		m_position += m_velocity * dt;
	}
	else
	{
		m_velocity = Vec2(0.f,0.f);
	}
}


void Player::RotateTurret()
{
	float dt = (float)m_game->m_gameClock->GetDeltaSeconds();
	InputSystem* g_theInput = g_engine->m_input;
	XboxController const& controller = g_engine->m_input->GetController(0);
	Vec2 aimDir(0.f, 0.f);

	//Controller(Right Stick)
	Vec2 rightStickDir = controller.GetRightStick().GetPosition();
	if (rightStickDir.GetLengthSquared() > 0.f)
	{
		aimDir = rightStickDir;
	}

	//Keyboard(IJKL)
	if (g_theInput->IsKeyDown('I')) 
		aimDir.y += 1.f;
	if (g_theInput->IsKeyDown('K')) 
		aimDir.y -= 1.f;
	if (g_theInput->IsKeyDown('J')) 
		aimDir.x -= 1.f;
	if (g_theInput->IsKeyDown('L')) 
		aimDir.x += 1.f;

	//Rotation Calculation
	if (aimDir.GetLengthSquared() > 0.f)
	{
		m_goalTurretOrientation = aimDir.GetOrientationDegrees();
		m_turretOrientationDegrees = GetTurnedTowardDegrees(m_turretOrientationDegrees, m_goalTurretOrientation,m_turretTurnSpeed * dt);
		m_turretRelativeOrientation = m_turretOrientationDegrees - m_orientationDegrees;
	}
	else
	{
		m_turretOrientationDegrees = m_orientationDegrees + m_turretRelativeOrientation;
	}
}


void Player::HandleShooting()
{
	InputSystem* input = g_engine->m_input;
	XboxController const& controller = input->GetController(0);

	bool isSpacePressed = input->IsKeyDown(KEYCODE_SPACE);
	bool isTriggerPressed = controller.GetRightTrigger() > 0.1f;

	if ((isSpacePressed || isTriggerPressed) && m_timeSinceLastShot >= m_fireCooldown)
	{
		Vec2 forward = Vec2::MakeFromPolarDegrees(m_turretOrientationDegrees);
		Vec2 muzzlePos = m_position + forward * 0.2f; 
		Vec2 bulletVel = forward * 7.5f;

		m_game->m_currentMap->SpawnNewEntity(ENTITY_TYPE_GOOD_BULLET, muzzlePos, m_turretOrientationDegrees);
		g_engine->m_audio->StartSound(m_game->m_shootSound, false, 0.25f, 0.0f, 1.0f, false);

		m_timeSinceLastShot = 0.f;
	}
}

void Player::RenderTank() const
{
	std::vector<Vertex> tankTextureVerts;
	AABB2 quad( -0.5f, -0.5f, 0.5f, 0.5f );
	AddVertsForAABB2(tankTextureVerts, quad, Rgba8( 255, 255, 255, 255 ), Vec2(0.f, 0.f), Vec2(1.f,1.f));
	TransformVertexArrayXY3D((int)tankTextureVerts.size(), tankTextureVerts.data(), 1.f, m_orientationDegrees, m_position);
	g_engine->m_render->BindTexture( m_tankTexture );
	g_engine->m_render->DrawVertexArray( tankTextureVerts );
}


void Player::RenderTurret() const
{
	std::vector<Vertex> turretTextureVerts;
	AABB2 quad( -0.5f, -0.5f, 0.5f, 0.5f );
	AddVertsForAABB2( turretTextureVerts, quad, Rgba8(255, 255, 255, 255), Vec2(0.f, 0.f), Vec2(1.f,1.f));
	TransformVertexArrayXY3D((int)turretTextureVerts.size(), turretTextureVerts.data(), 1.f, m_turretOrientationDegrees, m_position);
	g_engine->m_render->BindTexture( m_turretTexture );
	g_engine->m_render->DrawVertexArray( turretTextureVerts );
}


void Player::RenderStatus() const
{
	g_engine->m_render->BindTexture(nullptr);
	if (m_game->m_noClip)
	{
		DebugDrawRing(m_position, m_cosmeticRadius + 0.1f, 0.025f, Rgba8(255, 255, 255));
	}
	if (m_game->m_isInvincible)
	{
		DebugDrawRing(m_position, m_cosmeticRadius - 0.1f, 0.025f, Rgba8(0, 150, 255));
	}
}

void Player::CreateorGetTextures()
{
	m_tankTexture = g_engine->m_render->CreateOrGetTextureFromFile( "Data/Images/PlayerTankBase.png" );
	m_turretTexture = g_engine->m_render->CreateOrGetTextureFromFile("Data/Images/PlayerTankTop.png");
}

