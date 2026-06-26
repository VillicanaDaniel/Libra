#pragma once
#include "Engine/Core/Engine.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Renderer/Camera.hpp"

class PlayerShip;
class App;
class Game;


extern App* g_theApp;


class App
{
	public:
		App();
		~App();

		void RunMainLoop();
		void RunFrame();
		void Update();
		void Render() const;

		void SetIsQuitting();
		static bool SetIsQuittingEvent(EventArgs &args);
		bool IsQuitting() const;
		void UpdateFromKeyboard();
		void UpdateFromController();
		static bool testFunction(EventArgs& args);

	public:
		Camera* m_gameCamera = nullptr;
		Game* m_game = nullptr;

		bool m_isQuitting = false;
		bool m_isPaused = false;
		bool m_isSlowMo = false;
		bool m_isFastMo = false;
		bool m_pauseAfterNextUpdate = false;

		float m_lastFrameTime = 0.f;
		float m_timeScale = 1.f;

		Clock* m_systemClock = nullptr;
};