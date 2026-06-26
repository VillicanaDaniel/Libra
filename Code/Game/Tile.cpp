#include "Game/Tile.hpp"
#include "Engine/Renderer/Texture.hpp"
#include <cmath>

TileDef TileDef:: s_tileDefs[NUM_TILE_TYPES];
TileDef g_tileDefs[NUM_TILE_TYPES];

Tile::Tile(const IntVec2& tileCoords, TileType type)
	: m_tileCoords(tileCoords)
	, m_type(type)
{
}

bool Tile::isTileSolid() const
{
	TileDef const& myDef = TileDef::s_tileDefs[m_type];
	return myDef.m_isSolid;
}

void TileDef::InitializeTileDefs(SpriteSheet* sheet)
{
	s_tileDefs[GRASS].m_isSolid = false;
	s_tileDefs[STONE].m_isSolid = true;
	s_tileDefs[STONE_FLOOR].m_isSolid = false;
	s_tileDefs[MUD].m_isSolid = false;
	s_tileDefs[METAL].m_isSolid = true;
	s_tileDefs[SPAWN_TILE].m_isSolid = false;
	s_tileDefs[EXIT_TILE].m_isSolid = false;

	s_tileDefs[GRASS].m_tint = Rgba8(255, 255, 255, 255);
	s_tileDefs[STONE].m_tint = Rgba8(255, 255, 255, 255);
	s_tileDefs[STONE_FLOOR].m_tint = Rgba8(150, 150, 150, 150);
	s_tileDefs[MUD].m_tint = Rgba8(255, 255, 255, 255);
	s_tileDefs[METAL].m_tint = Rgba8(255, 255, 255, 255);
	s_tileDefs[SPAWN_TILE].m_tint = Rgba8(255, 255, 255, 255);
	s_tileDefs[EXIT_TILE].m_tint = Rgba8(255, 255, 255, 255);

	s_tileDefs[GRASS].m_UVs = sheet->GetSpriteDef(1).GetUVs();
	s_tileDefs[STONE].m_UVs = sheet->GetSpriteDef(17).GetUVs();
	s_tileDefs[STONE_FLOOR].m_UVs = sheet->GetSpriteDef(51).GetUVs();
	s_tileDefs[MUD].m_UVs = sheet->GetSpriteDef(21).GetUVs();
	s_tileDefs[METAL].m_UVs = sheet->GetSpriteDef(53).GetUVs();
	s_tileDefs[SPAWN_TILE].m_UVs = sheet->GetSpriteDef(56).GetUVs();
	s_tileDefs[EXIT_TILE].m_UVs = sheet->GetSpriteDef(57).GetUVs();
}


