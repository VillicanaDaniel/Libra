#pragma once
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Renderer/Camera.hpp"
#include <vector>

class Map;
class Player;
class Texture;
class SpriteSheet;
class SpriteAnimDefinition;
class SpriteDef;
class BitmapFont;
class Clock;


class Game
{
public:
	Game();
	~Game();

	void Update();
	void Render() const;

	void RenderAttractMode() const;
	void RenderWinScreen() const;
	void RenderPaused() const;
	void RenderGameOver() const;

	void UpdateFromKeyboard();
	void UpdateFromController();

	void InitializeMaps();
	void SwitchToNextMap();
	void CheckForMapTransition();


	void CreateSounds();
	void CreateorGetTextures();

	static bool testFunction(EventArgs& args);

public:
	std::vector<Map*> m_maps;
	int m_currentMapIndex;
	Map* m_currentMap = nullptr;

	float m_gameOverCounter;
	bool m_isVictory;

	float m_numTilesinViewVertically;
	Camera* m_screenCamera = nullptr;
	Camera* m_worldCamera = nullptr;
	Camera* m_attractCamera = nullptr;

	BitmapFont* g_testFont = nullptr;

	bool m_isInvincible = false;
	bool m_noClip = false;
	bool m_viewWorld = false;
	bool m_debugMode = false;
	bool m_viewDijkstra = false;

	Clock* GetClock() const;
	Clock* m_gameClock = nullptr;

	//----------------------------
	SpriteAnimDefinition* m_explosionAnim = nullptr;
	SpriteDef const* m_currentExplosionFrame = nullptr;
	float m_explosionElapsedTime = 0.f;
	//----------------------------

	enum GameState
	{
		GAMESTATE_ATTRACT,
		GAMESTATE_PLAYING,
		GAMESTATE_GAMEOVER,
		GAMESTATE_GAMEOVER_PENDING,
		NUM_GAME_MODES,
	};

	GameState m_gamestate = GAMESTATE_ATTRACT;

	SoundID m_gameMusic;
	SoundID m_attractMusic;
	SoundID m_startGameSound;
	SoundID m_shootSound;
	SoundID m_deathSound;
	SoundID m_pauseSound;
	SoundID m_winSound;
	SoundID m_spottedSound;
	SoundPlaybackID attractMusic;
	SoundPlaybackID gameMusic;
	SoundPlaybackID startGameSound;

	Texture *m_leoTexture;
	Texture *m_scorpioBaseTexture;
	Texture *m_scorpioTurretTexture;
	Texture *m_ariesTexture;
	Texture *m_winTexture;
	Texture *m_gameOverTexture;
	Texture *m_attractTexture;

	Texture *m_goodBulletTexture;
	Texture *m_evilBulletTexture;
	Texture *m_evilBoltTexture;

	SpriteSheet* g_terrainSpriteSheet = nullptr;
	SpriteSheet* m_explosionSheet = nullptr;
};