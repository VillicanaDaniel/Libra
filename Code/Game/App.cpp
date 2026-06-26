#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "ThirdParty/TinyXML2/tinyxml2.h"
#include "Engine/Core/XmlUtils.hpp" 


App* g_theApp = nullptr;


App::App()
{
	g_theApp = this; 
	tinyxml2::XMLDocument doc;
	doc.LoadFile("Data/GameConfig.xml");

	if (!doc.Error())
	{
		XmlElement* root = doc.RootElement();
		if (root)
		{
			g_gameConfigBlackboard.PopulateFromXmlElementAttributes(*root);
		}
	}
//---------------------------------------
	EngineConfig config;
	config.m_windowConfig.m_clientAspect =
		g_gameConfigBlackboard.GetValue("windowAspect", 16.f / 9.f);
	config.m_windowConfig.m_windowTitle = "Libra Foundation";
	g_engine = new Engine( config );

	m_systemClock = new Clock();
	m_systemClock->SetTimeScale(1.0);

	m_game = new Game();

	m_lastFrameTime = static_cast<float>(GetCurrentTimeSeconds());

	g_engine->m_event->SubscribeEventCallbackFunction("Quit", &App::SetIsQuittingEvent);
	g_engine->m_event->SubscribeEventCallbackFunction("Camera", &App::testFunction);
}

App::~App()
{
	delete m_game;
	m_game = nullptr;

	delete g_engine;
	g_engine = nullptr;
}


void App::RunMainLoop()
{
	while (!IsQuitting())
	{
		RunFrame();
	}
}


void App::RunFrame()
{
	m_systemClock->Tick();
// 	float timeNow = (float) GetCurrentTimeSeconds();
// 	float deltaSeconds = timeNow - m_lastFrameTime;
// 	m_lastFrameTime = timeNow;

	g_engine->BeginFrame();
	Update();		
	Render();		// #SD1ToDo: ...becomes just Render();		once this function becomes App::Run()
	g_engine->EndFrame(); // Allow engine subsystems to do post-frame stuff

}


void App::Update()
{
	InputSystem* input = g_engine->m_input;
	UpdateFromKeyboard();
	UpdateFromController();

	for (int id = 0; id < 4; ++id)
	{
		input->GetController(id);
	}

	//Cases for pause and slow, will be changed later
	if (m_isPaused == true)
		m_game->GetClock()->SetTimeScale(0.f);
	else if (m_isSlowMo == true)
		m_game->GetClock()->SetTimeScale(0.1f);
	else if (m_isFastMo == true)
		m_game->GetClock()->SetTimeScale(4.f);
	else if (m_isPaused == false)
		m_game->GetClock()->SetTimeScale(1.f);
	else if (m_isSlowMo == false)
		m_game->GetClock()->SetTimeScale(1.f);
	else if (m_isFastMo == false)
		m_game->GetClock()->SetTimeScale(1.f);

	if (m_pauseAfterNextUpdate)
	{
		m_game->GetClock()->SetTimeScale(1.f);
		m_pauseAfterNextUpdate = false;
		m_isPaused = true;
	}
	m_game->Update();
}


void App::Render() const
{
	g_engine->m_render->ClearScreen(Rgba8(0, 0, 0, 255));

	m_game->Render();
}


void App::SetIsQuitting()
{
	m_isQuitting = true;
}


bool App::SetIsQuittingEvent([[maybe_unused]]EventArgs &args)
{
	g_theApp->m_isQuitting = true;
	return true;
}


bool App::IsQuitting() const
{
	return m_isQuitting;
}


void App::UpdateFromKeyboard()
{
	InputSystem* input = g_engine->m_input;

	m_isSlowMo = input->IsKeyDown('T');

	m_isFastMo = input->IsKeyDown('Y');

	if (input->WasKeyJustPressed('P') && m_game->m_gamestate == Game::GAMESTATE_PLAYING)
	{
		g_engine->m_audio->StartSound(m_game->m_pauseSound, false, 0.5f, 0.0f, 1.0f, false);
		m_isPaused = !m_isPaused;
	}

	if (input->WasKeyJustPressed('O'))
	{
		m_isPaused = false;
		m_pauseAfterNextUpdate = true;
	}

	if (input->WasKeyJustPressed(KEYCODE_F8)) 
	{
		delete m_game;
		m_game = new Game();
	}

	if (m_game->m_isVictory && input->WasKeyJustPressed(' '))
	{
		delete m_game;
		m_game = new Game();
	}
}

void App::UpdateFromController()
{
	XboxController const& controller = g_engine->m_input->GetController(0);

		if (m_game->m_isVictory && controller.WasButtonJustPressed(XboxButtonID::START))
		{
			delete m_game;
			m_game = new Game();
		}
}

bool App::testFunction(EventArgs& args)
{
	App* app = g_theApp;
	if (app == nullptr)
		return false;

	Game* game = app->m_game;
	if (game == nullptr)
		return false;

	float newTileViewY = args.GetValue("arg1", game->m_numTilesinViewVertically);

	game->m_numTilesinViewVertically = newTileViewY;

	game->m_worldCamera->SetOrthographicView(
		Vec2(0.f, 0.f),
		Vec2(game->m_numTilesinViewVertically * 2.f,
			game->m_numTilesinViewVertically)
	);

	g_engine->m_devConsole->AddLine(
		DevConsole::COLOR_INFO_MINOR,
		Stringf("Camera view set to 12 tiles high", newTileViewY)
	);

	return true;
}

