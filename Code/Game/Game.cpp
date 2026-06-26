#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"
#include "Game/Map.hpp"
#include "Game/Player.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/TileHeatMap.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/SimpleTriangleFont.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Core/Image.hpp"

Game::Game()
{	
	CreateSounds();
	CreateorGetTextures();
	m_gamestate = GAMESTATE_ATTRACT;
	TileDef::InitializeTileDefs(g_terrainSpriteSheet);
	InitializeMaps();

	m_screenCamera = new Camera();
	m_worldCamera = new Camera();
	m_attractCamera = new Camera();

	m_screenCamera->SetOrthographicView( Vec2(0, 0), Vec2((float)m_currentMap->m_dimensions.y * 2.f, (float)m_currentMap->m_dimensions.y) );
	m_worldCamera->SetOrthographicView( Vec2(0.f, 0.f), Vec2(m_numTilesinViewVertically * 2.f, m_numTilesinViewVertically) );
	m_attractCamera->SetOrthographicView( Vec2(0.f,0.f), Vec2(100.f, 50.f) );

	attractMusic = g_engine->m_audio->StartSound(m_attractMusic, true, 0.25f, 0.0f, 1.0f, false);

	g_testFont = g_engine->m_render->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont"); // DO NOT SPECIFY FILE .EXTENSION!!  (Important later )

	WORLD_SIZE_X = g_gameConfigBlackboard.GetValue("WORLD_SIZE_X", 30.f);
	WORLD_SIZE_Y = g_gameConfigBlackboard.GetValue("WORLD_SIZE_Y", 20.f);

	WORLD_CENTER_X = WORLD_SIZE_X * 0.5f;
	WORLD_CENTER_Y = WORLD_SIZE_Y * 0.5f;

	SCREEN_SIZE_X = g_gameConfigBlackboard.GetValue("SCREEN_SIZE_X", 10.f);
	SCREEN_SIZE_Y = g_gameConfigBlackboard.GetValue("SCREEN_SIZE_Y", 5.f);

	m_currentMapIndex = g_gameConfigBlackboard.GetValue("currentMapIndex", 0);

	m_gameOverCounter = g_gameConfigBlackboard.GetValue("gameOverCounter", 2.f);
	m_isVictory = g_gameConfigBlackboard.GetValue("isVictory", false);

	m_numTilesinViewVertically = g_gameConfigBlackboard.GetValue("numTilesinViewVertically", 8.f);

	m_gameClock = new Clock(g_theApp->m_systemClock);
}


Game::~Game()
{
	delete m_currentMap;
	m_currentMap = nullptr;

	delete m_screenCamera;
	m_screenCamera = nullptr;

	delete m_worldCamera;
	m_worldCamera = nullptr;

	delete m_attractCamera;
	m_attractCamera = nullptr;

	g_engine->m_audio->StopSound(attractMusic);
	g_engine->m_audio->StopSound(gameMusic);
	g_engine->m_audio->StopSound(startGameSound);
}


void Game::Update()
{
	float dt = (float)m_gameClock->GetDeltaSeconds();
	UpdateFromKeyboard();
	UpdateFromController();

	if (m_gamestate == GAMESTATE_ATTRACT || m_isVictory)
	{ 
			m_explosionElapsedTime += dt;
			m_currentExplosionFrame = &m_explosionAnim->GetSpriteDefAtTime(m_explosionElapsedTime);
		return;
	}

	if (m_currentMap->m_player && m_currentMap->m_player->m_isDead)
	{
		if (m_gamestate != GAMESTATE_GAMEOVER_PENDING &&
			m_gamestate != GAMESTATE_GAMEOVER)
		{
			m_gamestate = GAMESTATE_GAMEOVER_PENDING;
			m_gameOverCounter = 2.f;
		}
		else if (m_gamestate == GAMESTATE_GAMEOVER_PENDING)
		{
			m_gameOverCounter -= dt;

			if (m_gameOverCounter <= 0.f)
			{
				m_gamestate = GAMESTATE_GAMEOVER;
				return;
			}
		}
	}

	else if (m_currentMap->m_player && !m_currentMap->m_player->m_isDead)
	{
		m_gamestate = GAMESTATE_PLAYING;
	}

	if (m_gamestate == GAMESTATE_GAMEOVER)
		return;

	m_currentMap->Update();
	CheckForMapTransition();
}


void Game::Render() const
{	
	if (m_gamestate == GAMESTATE_ATTRACT)
	{
		RenderAttractMode();
		return;
	}

	if (m_isVictory)
	{
		RenderWinScreen();
		return;
	}

	m_currentMap->Render();
	if (g_theApp->m_isPaused)
	{
		RenderPaused();
	}

	if (m_gamestate == GAMESTATE_GAMEOVER)
		RenderGameOver();

	AABB2 consoleBounds;
	if (m_gamestate == GAMESTATE_ATTRACT)
	{
		g_engine->m_render->BeginCamera(*m_attractCamera);
		consoleBounds = AABB2(
			Vec2(0.f, 0.f),
			m_attractCamera->GetOrthographicTopRight());
		g_engine->m_devConsole->Render(consoleBounds);
	}
	if (!m_viewWorld)
	{
		g_engine->m_render->BeginCamera(*m_worldCamera);
		consoleBounds = AABB2(
			Vec2(m_worldCamera->GetOrthographicTopRight().x - (m_numTilesinViewVertically * 2.f), m_worldCamera->GetOrthographicTopRight().y - m_numTilesinViewVertically),
			m_worldCamera->GetOrthographicTopRight());
		g_engine->m_devConsole->Render(consoleBounds);
	}
	else
	{
		g_engine->m_render->BeginCamera(*m_screenCamera);
		consoleBounds = AABB2(
			Vec2(0.f, 0.f),
			m_screenCamera->GetOrthographicTopRight());
		g_engine->m_devConsole->Render(consoleBounds);
	}
	
	g_engine->m_render->EndCamera(*m_attractCamera);
	g_engine->m_render->EndCamera(*m_worldCamera);
	g_engine->m_render->EndCamera(*m_screenCamera);
}


void Game::RenderAttractMode() const
{
	std::vector<Vertex> testVerts;

	AABB2 spriteBounds;
	spriteBounds.m_mins = Vec2(0.f, 0.f);
	spriteBounds.m_maxs = Vec2(100.f, 50.f);

	AddVertsForAABB2(testVerts, spriteBounds, Rgba8(255, 255, 255, 255), Vec2(0.f, 0.f), Vec2(1.f, 1.f));

	TransformVertexArrayXY3D((int)testVerts.size(), testVerts.data(), 1.f, 0.f, Vec2(0.f, 0.f));

	g_engine->m_render->BindTexture(m_attractTexture);
	g_engine->m_render->DrawVertexArray(testVerts);

	g_engine->m_render->BeginCamera(*m_attractCamera);

	std::vector<Vertex> textBoxVerts;
	AABB2 textBox;
	textBox.m_mins = Vec2(10.f, 20.f);
	textBox.m_maxs = Vec2(30.f, 40.f);

	AddVertsForAABB2(textBoxVerts, textBox, Rgba8(0, 0, 0), Vec2(0.f, 0.f), Vec2(1.f, 1.f));
	TransformVertexArrayXY3D((int)textBoxVerts.size(), textBoxVerts.data(), 1.f, 0.f, Vec2(0.f, 0.f));
	g_engine->m_render->BindTexture(nullptr);
	g_engine->m_render->DrawVertexArray(textBoxVerts);

	DebugDrawRing(Vec2(50.f,25.f), 10.f, 1.f, Rgba8(255,0,0));

	std::vector<Vertex> textVerts;
	g_testFont->AddVertsForText2D(textVerts, Vec2(50.f, 10.f), 1.f, "Hello, world");
	g_testFont->AddVertsForText2D(textVerts, Vec2(75.f, 30.f), 0.5f, "It's nice to have options!", Rgba8(255,0,0), 0.6f);
	g_testFont->AddVertsForTextInBox2D(textVerts, "This is a really long string \n that has multiple lines", textBox, 10.f,Rgba8(255,255,255), 1.0f, Vec2(0.0,0.5),SHRINK_TO_FIT);
	g_testFont->AddVertsForTextInBox2D(textVerts, "This is a really long string \n that will overrun", textBox, 1.f,Rgba8(255,0,255), 1.0f, Vec2(0.0,0.5),OVERRUN);
	g_engine->m_render->BindTexture(&g_testFont->GetTexture());
	g_engine->m_render->DrawVertexArray(textVerts);

	//--------------------------------------------------------------------------------
	// Animation Test
	//--------------------------------------------------------------------------------
	if (m_currentExplosionFrame != nullptr)
	{
		std::vector<Vertex> verts;
		Vec2 uvMins, uvMaxs;
		m_currentExplosionFrame->GetUVs(uvMins, uvMaxs);

		AABB2 bounds(Vec2(40.f, 20.f), Vec2(60.f, 40.f));

		AddVertsForAABB2(verts, bounds, Rgba8(255, 255, 255), uvMins, uvMaxs);

		g_engine->m_render->BindTexture(&m_currentExplosionFrame->GetTexture());
		g_engine->m_render->DrawVertexArray((int)verts.size(), verts.data());
	}

	g_engine->m_render->EndCamera(*m_attractCamera);
}


void Game::RenderWinScreen() const
{
	g_engine->m_render->BeginCamera(*m_attractCamera);
	std::vector<Vertex> tankTextureVerts;
	AABB2 quad(0.f, 0.0f, 100.f, 50.f);
	AddVertsForAABB2(tankTextureVerts, quad, Rgba8(255, 255, 255, 255), Vec2(0.f, 0.f), Vec2(1.f,1.f));
	TransformVertexArrayXY3D((int)tankTextureVerts.size(), tankTextureVerts.data(), 1.f, 0.f, Vec2(1.f,1.f));
	g_engine->m_render->BindTexture(m_winTexture);
	g_engine->m_render->DrawVertexArray(tankTextureVerts);
}

void Game::RenderPaused() const
{
	std::vector<Vertex> pauseVerts;
	AABB2 quad(0.f, 0.f, (float)m_currentMap->m_dimensions.x, (float)m_currentMap->m_dimensions.y);
	AddVertsForAABB2(pauseVerts, quad, Rgba8(255, 255, 255, 255), Vec2(0.f, 0.f), Vec2(1.f,1.f));
	for (int pauseVertIndex = 0; pauseVertIndex < pauseVerts.size(); pauseVertIndex++)
	{
		pauseVerts[pauseVertIndex].m_color = Rgba8(128, 128, 128, 100);
	}

	g_engine->m_render->DrawVertexArray(pauseVerts);
}
	

void Game::RenderGameOver() const
{
	g_engine->m_render->BeginCamera(*m_worldCamera);

	Vec2 view = m_worldCamera->GetOrthographicTopRight();
	AABB2 quad(view.x - m_numTilesinViewVertically * 2.f, view.y - m_numTilesinViewVertically, view.x, view.y);

	std::vector<Vertex> verts;
	AddVertsForAABB2(verts, quad, Rgba8(255, 255, 255, 255), Vec2(0.f, 0.f), Vec2(1.f,1.f));

	g_engine->m_render->BindTexture(m_gameOverTexture);
	g_engine->m_render->DrawVertexArray((int)verts.size(), verts.data());

	g_engine->m_render->EndCamera(*m_worldCamera);
}

//---------------------------------------------------------------------------------------------------------------------------------
// INPUT
//---------------------------------------------------------------------------------------------------------------------------------
void Game::UpdateFromKeyboard()
{
	InputSystem* input = g_engine->m_input;

	if (input->WasKeyJustPressed('P') && m_gamestate == GAMESTATE_ATTRACT)
	{
		m_gamestate = GAMESTATE_PLAYING;
		g_engine->m_audio->StopSound(attractMusic);
		startGameSound = g_engine->m_audio->StartSound(m_startGameSound, false, 0.25f, 0.0f, 1.0f, false);
		gameMusic = g_engine->m_audio->StartSound(m_gameMusic, true, 0.25f, 0.0f, 1.0f, false);
	}
	if (input->WasKeyJustPressed(KEYCODE_F1))
	{
		m_debugMode = !m_debugMode;
	}
	if (input->WasKeyJustPressed(KEYCODE_F2))
	{
		m_isInvincible = !m_isInvincible;
	}
	if (input->WasKeyJustPressed(KEYCODE_F3))
	{
		m_noClip = !m_noClip;
	}
	if (input->WasKeyJustPressed(KEYCODE_F4))
	{
		m_viewWorld = !m_viewWorld;
	}
	if (input->WasKeyJustPressed(KEYCODE_F5))
	{
		m_viewDijkstra = !m_viewDijkstra;
	}
	if (input->WasKeyJustPressed(KEYCODE_F6))
	{
		g_engine->m_devConsole->Execute("Camera 12");
	}
	if (input->WasKeyJustPressed(KEYCODE_F9))
	{
		SwitchToNextMap();
	}
	if (input->WasKeyJustPressed(KEYCODE_ESC) && m_gamestate == GAMESTATE_ATTRACT)
	{
		g_theApp->SetIsQuitting();
	}
	if (input->WasKeyJustPressed(KEYCODE_ESC) && m_gamestate != GAMESTATE_ATTRACT)
	{
		g_engine->m_audio->StopSound(gameMusic);
		g_engine->m_audio->StopSound(startGameSound);
		m_gamestate = GAMESTATE_ATTRACT;
	}
	if (input->WasKeyJustPressed(KEYCODE_ESC))
	{
		m_gamestate = GAMESTATE_ATTRACT;
		g_engine->m_audio->StopSound(attractMusic);
		attractMusic = g_engine->m_audio->StartSound(m_attractMusic, true, 0.25f, 0.0f, 1.0f, false);
	}
	if (input->WasKeyJustPressed('N') && m_currentMap->m_player->m_isDead)
	{
		m_currentMap->m_player->m_isDead = false;
		m_currentMap->m_player->m_health = 10;
	}
	if (input->WasKeyJustPressed('M'))
	{
		g_engine->m_devConsole->AddLine(DevConsole::COLOR_INFO_MAJOR, "And I can add text to it!");
	}
}


void Game::UpdateFromController()
{
	XboxController const& controller = g_engine->m_input->GetController(0);

	if (controller.WasButtonJustPressed(XboxButtonID::START) && (m_gamestate == GAMESTATE_PLAYING))
	{
		g_theApp->m_isPaused = !g_theApp->m_isPaused;
	}
	if (controller.WasButtonJustPressed(XboxButtonID::START) && m_gamestate == GAMESTATE_ATTRACT)
	{
		m_gamestate = GAMESTATE_PLAYING;
		g_engine->m_audio->StopSound(attractMusic);
		startGameSound = g_engine->m_audio->StartSound(m_startGameSound, false, 0.25f, 0.0f, 1.0f, false);
		gameMusic = g_engine->m_audio->StartSound(m_gameMusic, true, 0.25f, 0.0f, 1.0f, false);
	}
	if (controller.WasButtonJustPressed(XboxButtonID::START) && m_currentMap->m_player->m_isDead)
	{
		m_currentMap->m_player->m_isDead = false;
		m_currentMap->m_player->m_health = 10;
	}
	if (controller.WasButtonJustPressed(XboxButtonID::B) && m_gamestate == GAMESTATE_ATTRACT)
	{
		g_theApp->SetIsQuitting();
	}
	if (controller.WasButtonJustPressed(XboxButtonID::B) && m_gamestate == GAMESTATE_PLAYING)
	{
		m_gamestate = GAMESTATE_ATTRACT;
		g_engine->m_audio->StopSound(gameMusic);
		g_engine->m_audio->StopSound(startGameSound);
	}
}


void Game::InitializeMaps()
{
	for (Map* map : m_maps)
		delete map;
	m_maps.clear();

	// Map 1
	Map* map1 = new Map(this);
	map1->Initialize(30, 20 , TileType::GRASS, TileType::STONE, TileType::STONE, TileType::GRASS, TileType::STONE_FLOOR, TileType::STONE);
	map1->SpawnNewEntity(ENTITY_TYPE_GOOD_PLAYER, Vec2(1.5f, 1.5f));
	map1->SpawnNewEntity(ENTITY_TYPE_EVIL_SCORPIO, Vec2(20.5f, 10.5f));
	map1->SpawnNewEntity(ENTITY_TYPE_EVIL_ARIES, Vec2(5.5f, 5.5f));

	// Map 2
	Map* map2 = new Map(this);
	map2->Initialize(40, 20, TileType::GRASS, TileType::STONE, TileType::STONE, TileType::MUD, TileType::STONE_FLOOR, TileType::METAL);
	map2->SpawnNewEntity(ENTITY_TYPE_EVIL_ARIES, Vec2(10.5f, 10.5f));
	map2->SpawnNewEntity(ENTITY_TYPE_EVIL_LEO, Vec2(25.5f, 12.5f));
	map2->SpawnNewEntity(ENTITY_TYPE_EVIL_SCORPIO, Vec2(30.5f, 15.5f));

	// Map 3
	Map* map3 = new Map(this);
	map3->Initialize(50, 30, TileType::GRASS, TileType::STONE, TileType::MUD, TileType::STONE, TileType::STONE_FLOOR, TileType::METAL);
	map3->SpawnNewEntity(ENTITY_TYPE_EVIL_ARIES, Vec2(15.5f, 20.5f));
	map3->SpawnNewEntity(ENTITY_TYPE_EVIL_ARIES, Vec2(30.5f, 10.5f));
	map3->SpawnNewEntity(ENTITY_TYPE_EVIL_LEO, Vec2(35.5f, 15.5f));
	map3->SpawnNewEntity(ENTITY_TYPE_EVIL_SCORPIO, Vec2(40.5f, 25.5f));

	m_maps.push_back(map1);
	m_maps.push_back(map2);
	m_maps.push_back(map3);

	m_currentMapIndex = 0;
	m_currentMap = m_maps[m_currentMapIndex];
}


void Game::SwitchToNextMap()
{
	if (m_currentMapIndex >= (int)m_maps.size() - 1)
	{
		m_isVictory = true;
		return;
	}

	Player* player = m_currentMap->m_player;

	m_currentMap->RemoveEntityFromMap(*player);
	m_currentMapIndex++;
	m_currentMap = m_maps[m_currentMapIndex];

	m_currentMap->m_player = player;
	m_currentMap->AddEntityToMap(*player);
	player->m_position = Vec2(1.5f, 1.5f);
}


void Game::CheckForMapTransition()
{
	if (!m_currentMap || !m_currentMap->m_player)
		return;

	Player* player = m_currentMap->m_player;
	Vec2 exitPos = m_currentMap->m_exitPosition;

	if (GetDistance2D(player->m_position, exitPos) < player->m_physicsRadius)
	{
		SwitchToNextMap();
	}
}


void Game::CreateSounds()
{
	m_gameMusic = g_engine->m_audio->CreateOrGetSound("Data/Audio/53. Tanks - Variation 1.mp3");
	m_attractMusic = g_engine->m_audio->CreateOrGetSound("Data/Audio/02. Title.mp3");
	m_startGameSound = g_engine->m_audio->CreateOrGetSound("Data/Audio/52. Tanks - Round Start.mp3");
	m_shootSound = g_engine->m_audio->CreateOrGetSound("Data/Audio/Shoot70.wav");
	m_deathSound = g_engine->m_audio->CreateOrGetSound("Data/Audio/ShipBoom.wav");
	m_pauseSound = g_engine->m_audio->CreateOrGetSound("Data/Audio/testSound.mp3");
	m_winSound = g_engine->m_audio->CreateOrGetSound("Data/Audio/52. Tanks - Round Start.mp3");
	m_spottedSound =g_engine->m_audio->CreateOrGetSound("Data/Audio/spottedSound.mp3");
}


void Game::CreateorGetTextures()
{
	m_leoTexture = g_engine->m_render->CreateOrGetTextureFromFile( "Data/Images/EnemyTank1.png" );
	m_scorpioBaseTexture = g_engine->m_render->CreateOrGetTextureFromFile( "Data/Images/EnemyTurretBase.png" );
	m_scorpioTurretTexture = g_engine->m_render->CreateOrGetTextureFromFile( "Data/Images/EnemyCannon.png" );
	m_ariesTexture = g_engine->m_render->CreateOrGetTextureFromFile( "Data/Images/EnemyAries.png" );
	m_winTexture = g_engine->m_render->CreateOrGetTextureFromFile( "Data/Images/VictoryScreen.jpg" );
	m_gameOverTexture = g_engine->m_render->CreateOrGetTextureFromFile( "Data/Images/YouDiedScreen.png" );
	m_attractTexture = g_engine->m_render->CreateOrGetTextureFromFile( "Data/Images/AttractScreen.png" );
	m_goodBulletTexture = g_engine->m_render->CreateOrGetTextureFromFile( "Data/Images/FriendlyBolt.png" );
	m_evilBulletTexture = g_engine->m_render->CreateOrGetTextureFromFile( "Data/Images/EnemyBullet.png" );
	m_evilBoltTexture = g_engine->m_render->CreateOrGetTextureFromFile( "Data/Images/EnemyBolt.png" );
	Texture* terrainTexture = g_engine->m_render->CreateOrGetTextureFromFile("Data/Images/Terrain_8x8.png");
	g_terrainSpriteSheet = new SpriteSheet(*terrainTexture, IntVec2(8, 8));
	
	Texture* explosionTexture = g_engine->m_render->CreateOrGetTextureFromFile("Data/Images/Explosion_5x5.png");
	m_explosionSheet = new SpriteSheet(*explosionTexture, IntVec2(5, 5));

	m_explosionAnim = new SpriteAnimDefinition(
		*m_explosionSheet,       // sprite sheet
		0,                       // start index
		24,                       // end index
		20.f,                     // fps
		SpriteAnimPlaybackType::ONCE
	);
}

Clock* Game::GetClock() const
{
	return m_gameClock;
}
