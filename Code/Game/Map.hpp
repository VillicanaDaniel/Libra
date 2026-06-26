#pragma once
#include "Game/Game.hpp"
#include "Game/Tile.hpp"
#include "Game/Entity.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/TileHeatMap.hpp"
#include <vector>

class Entity;
struct Vec2;

struct RaycastResult2D
{
	// Basic raycast result information (required)
	bool	m_didImpact = false;
	float	m_impactDist = 0.f;
	Vec2	m_impactPos;
	Vec2	m_impactNormal;

	// Original raycast information (optional)
	Vec2	m_rayStartPos;
	Vec2	m_rayFwdNormal;
	float	m_rayMaxLength = 1.f;
};

struct MapDef
{
	IntVec2 m_dimensions;
	TileType m_fillTileType;
	TileType m_edgeTileType;
	TileType m_sprinkle1TileType;
	TileType m_sprinkle2TileType;
	float m_sprinkle1Probability;
	float m_sprinkle2Probability;
};

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Map
{
public:
	Map(Game* Owner);
	~Map();

	void Initialize(int width, int height, TileType fill, TileType border, TileType sprinkleType1, TileType sprinkleType2, TileType bunkerFloor, TileType bunkerWall);
	void Update();
	void Render() const;

	void RenderTiles() const;
	void RenderEntities() const;
	void RenderDijkstraMap() const;

	void UpdateEntities();
	void UpdateCameras();

	void HandleBulletEntityCollisions();
	void HandleBulletWallCollisions();
	void HandleEntitytoEntityCollisions();
	void PushEntitiesOutOfEachOther(Entity &a, Entity &b);
	void PushEntitiesOutofSolidTile();
	void PushPlayerOutOfSolidTile();
	void DeleteGarbage();

	Tile* GetTileAtTileCoords(const IntVec2& tileCoords);
	IntVec2 GetTileCoordsForWorldPos(Vec2 const& worldPos) const;
	void SetTileType(const IntVec2& tileCoords, TileType type);
	bool IsTileSolid(IntVec2 const& tileCoords) const;
	bool IsPointInSolid(Vec2 const& point) const;

	Entity* SpawnNewEntity(EntityType type, Vec2 const& position, float orientationDegrees = 0.f);
	void AddEntityToMap(Entity& e);
	void RemoveEntityFromMap(Entity& e);

	RaycastResult2D RaycastVsTiles2D(Vec2 startPos, Vec2 fwdNormal, float maxDist);
	bool HasLineOfSight(Vec2 const& startPos, Vec2 const& endPos);

public:
	std::vector< Tile >		m_tiles;
	IntVec2					m_dimensions;

	std::vector< Entity* >	m_allEntities;
	EntityList m_entityListsByType[NUM_ENTITY_TYPES];

	Game* m_game = nullptr;
	Player* m_player = nullptr;
	TileHeatMap* m_dijkstraMap = nullptr;
	Vec2 m_exitPosition;

private:
	void PopulateTiles(TileType fill);
	void SetBorderTiles(TileType border);
	void PopulateSprinkleTypes( TileType sprinkleType1, TileType sprinkleType2);
	void GenerateWorms(TileType wormTile, int numWorms, int wormLength);
	void CreateReservedAreas(TileType bunkerFloor, TileType bunkerWall);
	void PopulateDijkstraMap(
		TileHeatMap& out_dijkstraMap,
		IntVec2 startCoords,
		float maxCost,
		bool treatWaterAsSolid
	);
	void FillUnreachableTiles();
	bool IsExitReachable() const;

};