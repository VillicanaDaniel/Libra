#pragma once
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/Rgba8.hpp"

struct IntVec2;
class SpriteSheet;

enum TileType
{
	TILE_TYPE_INVALID = -1,
	GRASS,
	GRASS_2,
	STONE,
	STONE_FLOOR,
	MUD,
	METAL,
	SPAWN_TILE,
	EXIT_TILE,
	NUM_TILE_TYPES
};

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class TileDef
{
public:
	bool m_isSolid = false;
	Rgba8 m_tint = Rgba8(255,255,255);
	AABB2 m_UVs = AABB2( Vec2(0.f,0.f), Vec2(1.f,1.f) );
	Rgba8 m_mapImageColor = Rgba8(0,0,0,0);

public:
	static void InitializeTileDefs(SpriteSheet* sheet);
	static TileDef s_tileDefs[NUM_TILE_TYPES];
};

class Tile
{
public:
	Tile() = default;
	Tile(const IntVec2& tileCoords, TileType type = TileType::GRASS);

	bool isTileSolid() const;

public:
	IntVec2 m_tileCoords;
	TileType m_type = TileType::GRASS;
};