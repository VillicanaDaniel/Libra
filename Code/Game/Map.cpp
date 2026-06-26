#include "Game/Map.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Player.hpp"
#include "Game/Scorpio.hpp"
#include "Game/Leo.hpp"
#include "Game/Aries.hpp"
#include "Game/Bullet.hpp"
#include "Game/Explosion.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

Map::Map(Game* Owner)
{
	m_game = Owner;
}


Map::~Map()
{
	for (Entity* e : m_allEntities)
	{
		delete e;
		e = nullptr;
	}

	delete m_dijkstraMap;
	m_dijkstraMap = nullptr;
}


void Map::Initialize(int width, int height, 
TileType fill, TileType border, 
TileType sprinkleType1, TileType sprinkleType2, 
TileType bunkerFloor, TileType bunkerWall)
{
	bool mapIsValid = false;

	while (!mapIsValid)
	{ 
	m_dimensions = IntVec2(width, height);
	m_tiles.clear();
	m_tiles.reserve(width * height);

	m_exitPosition.x = (float)width - 1.5f;
	m_exitPosition.y = (float)height - 1.5f;

	PopulateTiles(fill);
	GenerateWorms(sprinkleType1, 5,25);
	GenerateWorms(sprinkleType2, 6, 10);
	CreateReservedAreas(bunkerFloor, bunkerWall);
	SetBorderTiles(border);

	//Heat Map
	m_dijkstraMap = new TileHeatMap(m_dimensions);
	PopulateDijkstraMap(*m_dijkstraMap, IntVec2(1,1), 999.f, false);

	FillUnreachableTiles();

	mapIsValid = IsExitReachable();
	}
}


void Map::Update()
{
	UpdateCameras();
	UpdateEntities();

	HandleBulletEntityCollisions();
	HandleBulletWallCollisions();

	PushPlayerOutOfSolidTile();
	PushEntitiesOutofSolidTile();
	HandleEntitytoEntityCollisions();

	DeleteGarbage();
}


void Map::Render() const 
{
	RenderTiles();
	RenderEntities();

	if (m_game->m_viewDijkstra)
	{
		RenderDijkstraMap();
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Rendering Helpers
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Map::RenderTiles() const
{
	std::vector<Vertex> tileVerts;
	tileVerts.reserve(m_tiles.size() * 6);

	for (int tileIndex = 0; tileIndex < (int)m_tiles.size(); ++tileIndex)
	{
		const Tile& tile = m_tiles[tileIndex];
		const TileDef& def = TileDef::s_tileDefs[tile.m_type];

		Vec2 mins((float)tile.m_tileCoords.x, (float)tile.m_tileCoords.y);
		Vec2 maxs(mins.x + 1.f, mins.y + 1.f);
		AABB2 tileAABB2(mins, maxs);

		Vec2 uvMins = def.m_UVs.m_mins;
		Vec2 uvMaxs = def.m_UVs.m_maxs;

		AddVertsForAABB2(tileVerts, tileAABB2, def.m_tint, uvMins, uvMaxs);
	}

	g_engine->m_render->BindTexture(&m_game->g_terrainSpriteSheet->GetTexture());
	g_engine->m_render->DrawVertexArray((int)tileVerts.size(), tileVerts.data());
}


void Map::RenderEntities() const
{
	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++)
	{
		if (m_allEntities[entityIndex] != nullptr)
		{
			m_allEntities[entityIndex]->Render();
			if (m_game->m_debugMode)
			{
				m_allEntities[entityIndex]->RenderDebug();
			}
		}
	}
	g_engine->m_render->BindTexture(nullptr);
}


void Map::RenderDijkstraMap() const
{
	if (!m_dijkstraMap)
		return;

	std::vector<Vertex> tileVerts;
	tileVerts.reserve(m_tiles.size() * 6);

	const float maxDisplayCost = 50.f;
	const float unreachableCost = 999.f;

	for (int tileIndex = 0; tileIndex < (int)m_tiles.size(); ++tileIndex)
	{
		const Tile& tile = m_tiles[tileIndex];

		IntVec2 coords = tile.m_tileCoords;
		float heat = m_dijkstraMap->GetTileHeatValue(coords);

		Rgba8 heatColor;

		if (heat >= unreachableCost)
		{
			// unreachable
			heatColor = Rgba8(0, 0, 255, 220);
		}
		else if (heat == 0.f)
		{
			// start tile
			heatColor = Rgba8(0, 255, 0, 220);
		}
		else
		{
			float normalized = heat / maxDisplayCost;
			normalized = GetClamped(normalized, 0.f, 1.f);

			unsigned char grey = (unsigned char)(255.f * (1.f - normalized));
			heatColor = Rgba8(grey, grey, grey, 200);
		}

		Vec2 mins((float)coords.x, (float)coords.y);
		Vec2 maxs(mins.x + 1.f, mins.y + 1.f);

		AABB2 cellAABB(mins, maxs);

		AddVertsForAABB2(tileVerts, cellAABB, heatColor, Vec2(0.f, 0.f), Vec2(1.f, 1.f));
	}

	g_engine->m_render->BindTexture(nullptr);
	g_engine->m_render->DrawVertexArray((int)tileVerts.size(), tileVerts.data());
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Update Helpers
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::UpdateEntities()
{
	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++)
	{
		if (m_allEntities[entityIndex] != nullptr)
		{
			m_allEntities[entityIndex]->Update();
		}
	}
}


void Map::UpdateCameras()
{
	if (m_player == nullptr) 
		return;

	if (m_game->m_viewWorld)
	{
		float mapW = (float)m_dimensions.x;
		float mapH = (float)m_dimensions.y;

		float targetAspect = 2.0f;
		float viewW = mapW;
		float viewH = mapH;

		float mapAspect = mapW / mapH;

		if (mapAspect > targetAspect)
		{
			viewH = viewW / targetAspect;
		}
		else
		{
			viewW = viewH * targetAspect;
		}

		m_game->m_screenCamera->SetOrthographicView(Vec2(0.f, 0.f), Vec2(viewW, viewH));
		return;
	}

	//Player Tracking + Clamping
	Vec2 playerPos = m_player->m_position;

	Vec2 viewMins = playerPos - Vec2(m_game->m_numTilesinViewVertically,
		m_game->m_numTilesinViewVertically * 0.5f);
	Vec2 viewMaxs = viewMins + Vec2(m_game->m_numTilesinViewVertically * 2.f,
		m_game->m_numTilesinViewVertically);

	viewMins.x = GetClamped(viewMins.x, 0.f, m_dimensions.x - m_game->m_numTilesinViewVertically * 2.f);
	viewMins.y = GetClamped(viewMins.y, 0.f, m_dimensions.y - m_game->m_numTilesinViewVertically);

	viewMaxs = viewMins + Vec2(m_game->m_numTilesinViewVertically * 2.f,
		m_game->m_numTilesinViewVertically);

	m_game->m_worldCamera->SetOrthographicView(viewMins, viewMaxs);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Handling Entity Interactions
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Map::HandleBulletEntityCollisions()
{
	for (int entityIndex = 0; entityIndex < (int)m_allEntities.size(); entityIndex++)
	{
		Bullet* bullet = dynamic_cast<Bullet*>(m_allEntities[entityIndex]);
		if (!bullet || bullet->m_isDead)
			continue;

		for (int entityIndexRecipient = 0; entityIndexRecipient < (int)m_allEntities.size(); entityIndexRecipient++)
		{
			if (m_allEntities[entityIndexRecipient] == bullet || m_allEntities[entityIndexRecipient]->m_isDead)
				continue;
			if (!m_allEntities[entityIndexRecipient]->m_isHitByBullets)
				continue;
			if (m_allEntities[entityIndexRecipient]->m_faction == bullet->m_faction)
				continue;

			if (DoDiscsOverlap(bullet->m_position, bullet->m_physicsRadius, m_allEntities[entityIndexRecipient]->m_position, m_allEntities[entityIndexRecipient]->m_physicsRadius))
			{
				Aries* aries = dynamic_cast<Aries*>(m_allEntities[entityIndexRecipient]);

				//Aries Handling
				if (aries)
				{
					Vec2 incomingDir = -bullet->m_velocity.GetNormalized();

					if (IsPointInsideDirectedSector2D(bullet->m_position, aries->m_position, aries->GetForwardNormal(), 100.f, aries->m_cosmeticRadius))
					{
						Vec2 normal = aries->GetForwardNormal();
						bullet->m_velocity.Reflect(normal);
						bullet->m_orientationDegrees = Atan2Degrees(bullet->m_velocity.y, bullet->m_velocity.x);

						if (bullet->m_faction == FACTION_GOOD)
						{
							bullet->m_bouncesRemaining--;
							if (bullet->m_bouncesRemaining < 0)
								bullet->setDead();
						}
						continue;
					}
				}

				m_allEntities[entityIndexRecipient]->TakeDamage(1);
				bullet->setDead();
				break;
			}
		}
	}
}


void Map::HandleBulletWallCollisions()
{
	float dt = (float)m_game->m_gameClock->GetDeltaSeconds();
	for (Entity* e : m_allEntities)
	{
		Bullet* bullet = dynamic_cast<Bullet*>(e);
		if (!bullet || bullet->m_isDead)
			continue;

		// Predict next position
		Vec2 nextPos = bullet->m_position + (bullet->m_velocity * dt);

		// Compute bounding circle for the bullet
		float radius = bullet->m_cosmeticRadius;
		IntVec2 currentTileCoords = GetTileCoordsForWorldPos(bullet->m_position);

		// List of 8 neighbor directions to test for collision
		const IntVec2 neighborOffsets[8] = {
			IntVec2(1, 0), IntVec2(-1, 0),
			IntVec2(0, 1), IntVec2(0, -1),
			IntVec2(1, 1), IntVec2(-1, 1),
			IntVec2(1, -1), IntVec2(-1, -1)
		};

		bool didBounce = false;
		Vec2 normal(0.f, 0.f);

		for (int i = 0; i < 8; i++)
		{
			IntVec2 neighbor = currentTileCoords + neighborOffsets[i];
			Tile* tile = GetTileAtTileCoords(neighbor);
			if (!tile || !tile->isTileSolid())
				continue;

			AABB2 tileBounds((float)neighbor.x, (float)neighbor.y,
				(float)neighbor.x + 1.f, (float)neighbor.y + 1.f);

			// Get nearest point to bullet
			Vec2 nearestPoint = tileBounds.GetNearestPoint(nextPos);
			Vec2 displacement = nextPos - nearestPoint;
			float distance = displacement.GetLength();

			if (distance < radius)
			{
				// Compute bounce normal from the displacement vector
				if (distance < 0.0001f)
					displacement = Vec2(1.f, 0.f);

				normal = displacement.GetNormalized();

				// Reflect velocity
				bullet->m_velocity.Reflect(normal);
				bullet->m_orientationDegrees = Atan2Degrees(bullet->m_velocity.y, bullet->m_velocity.x);
				didBounce = true;
				break;
			}
		}

		if (didBounce)
		{
			if (bullet->m_faction == FACTION_GOOD)
			{
				bullet->m_bouncesRemaining--;
				if (bullet->m_bouncesRemaining < 0)
					bullet->setDead();
			}
			else
			{
				bullet->setDead();
			}
		}
		else
		{
			// No solid collision — move bullet normally
			bullet->m_position = nextPos;
		}
	}
}

void Map::HandleEntitytoEntityCollisions()
{
	for (int i = 0; i < (int)m_allEntities.size(); i++)
	{
		Entity* a = m_allEntities[i];
		if (a == nullptr || a->m_isDead)
			return;

		for (int j = i + 1; j < (int)m_allEntities.size(); j++)
		{
			Entity* b = m_allEntities[j];
			if (b == nullptr || b->m_isDead)
				return;

			if (DoDiscsOverlap(a->m_position, a->m_physicsRadius, b->m_position, b->m_physicsRadius))
			{
				PushEntitiesOutOfEachOther(*a, *b);
			}
		}
	}
}

void Map::PushPlayerOutOfSolidTile()
{
	Player* player = m_player;
	if (player == nullptr || m_game->m_noClip || player->m_isDead)
		return;

	Vec2 playerPos = player->m_position;
	float playerRadius = player->m_physicsRadius;
	int playerTileX = (int)floorf(playerPos.x);
	int playerTileY = (int)floorf(playerPos.y);

	//EAST, SOUTH, NORTH, WEST CHECKS
	const IntVec2 cardinalOffsets[4] = { IntVec2(0, 1), IntVec2(0, -1), IntVec2(1, 0), IntVec2(-1, 0) };
	for (int i = 0; i < 4; i++)
	{
		int tileX = playerTileX + cardinalOffsets[i].x;
		int tileY = playerTileY + cardinalOffsets[i].y;

		Tile* tile = GetTileAtTileCoords(IntVec2(tileX, tileY));
		if (!tile)
			continue;
		if (!tile->isTileSolid())
			continue;

		AABB2 tileBounds((float)tileX, (float)tileY, (float)tileX + 1.f, (float)tileY + 1.f);
		Vec2 nearestPoint = tileBounds.GetNearestPoint(playerPos);

		Vec2 displacement = playerPos - nearestPoint;
		float distanceToPlayer = displacement.GetLength();
		if (distanceToPlayer < 0.0001f)
		{
			displacement = Vec2(1.f, 0.f);
			distanceToPlayer = 0.0001f;
		}

		float overlap = playerRadius - distanceToPlayer;
		if (overlap > 0.f)
		{
			playerPos += (displacement / distanceToPlayer) * overlap;
		}
	}

	//NW, NE, SW, SE Checks
	const IntVec2 diagonalOffsets[4] = { IntVec2(1, 1), IntVec2(-1, 1), IntVec2(1, -1), IntVec2(-1, -1) };
	for (int i = 0; i < 4; i++)
	{
		int tileX = playerTileX + diagonalOffsets[i].x;
		int tileY = playerTileY + diagonalOffsets[i].y;

		Tile* tile = GetTileAtTileCoords(IntVec2(tileX, tileY));
		if (!tile)
			continue;
		if (!tile->isTileSolid())
			continue;

		AABB2 tileBounds((float)tileX, (float)tileY, (float)tileX + 1.f, (float)tileY + 1.f);
		Vec2 nearestPoint = tileBounds.GetNearestPoint(playerPos);

		Vec2 displacement = playerPos - nearestPoint;
		float distanceToPlayer = displacement.GetLength();
		if (distanceToPlayer < 0.0001f)
		{
			displacement = Vec2(1.f, 0.f);
			distanceToPlayer = 0.0001f;
		}

		float overlap = playerRadius - distanceToPlayer;
		if (overlap > 0.f)
		{
			playerPos += (displacement / distanceToPlayer) * overlap;
		}
	}

	player->m_position = playerPos;
}

void Map::PushEntitiesOutOfEachOther(Entity& a, Entity& b)
{
	bool isBmoved = a.m_doesPushEntities && b.m_isPushedByEntities;
	bool isAmoved = b.m_doesPushEntities && a.m_isPushedByEntities;

	if (!isAmoved && !isBmoved)
	{
		return;
	}

	if (isBmoved && isAmoved)
	{
		PushDiscsOutOfEachOther2D(a.m_position, a.m_physicsRadius, b.m_position, b.m_physicsRadius);
	}
	else if (isBmoved)
	{
		PushDiscOutOfFixedDisc2D(b.m_position, b.m_physicsRadius, a.m_position, a.m_physicsRadius);
	}
	else
	{
		PushDiscOutOfFixedDisc2D(a.m_position, a.m_physicsRadius, b.m_position, b.m_physicsRadius);
	}
}

void Map::PushEntitiesOutofSolidTile()
{
	for (Entity* e : m_allEntities)
	{
		if (e == nullptr || e->m_isDead || !e->m_isPushedByWalls)
			continue;

		if (m_game->m_noClip && e == m_player)
			continue;

		Vec2 entityPos = e->m_position;
		float entityRadius = e->m_physicsRadius;
		int entityTileX = (int)floorf(entityPos.x);
		int entityTileY = (int)floorf(entityPos.y);

		//EAST, SOUTH, NORTH, WEST CHECKS
		const IntVec2 cardinalOffsets[4] = { IntVec2(0, 1), IntVec2(0, -1), IntVec2(1, 0), IntVec2(-1, 0) };
		for (int i = 0; i < 4; i++)
		{
			int tileX = entityTileX + cardinalOffsets[i].x;
			int tileY = entityTileY + cardinalOffsets[i].y;

			Tile* tile = GetTileAtTileCoords(IntVec2(tileX, tileY));
			if (!tile)
				continue;
			if (!tile->isTileSolid())
				continue;

			AABB2 tileBounds((float)tileX, (float)tileY, (float)tileX + 1.f, (float)tileY + 1.f);
			Vec2 nearestPoint = tileBounds.GetNearestPoint(entityPos);

			Vec2 displacement = entityPos - nearestPoint;
			float distanceToEntity = displacement.GetLength();
			if (distanceToEntity < 0.0001f)
			{
				displacement = Vec2(1.f, 0.f);
				distanceToEntity = 0.0001f;
			}

			float overlap = entityRadius - distanceToEntity;
			if (overlap > 0.f)
			{
				entityPos += (displacement / distanceToEntity) * overlap;
			}
		}

		//NW, NE, SW, SE Checks
		const IntVec2 diagonalOffsets[4] = { IntVec2(1, 1), IntVec2(-1, 1), IntVec2(1, -1), IntVec2(-1, -1) };
		for (int i = 0; i < 4; i++)
		{
			int tileX = entityTileX + diagonalOffsets[i].x;
			int tileY = entityTileY + diagonalOffsets[i].y;

			Tile* tile = GetTileAtTileCoords(IntVec2(tileX, tileY));
			if (!tile)
				continue;
			if (!tile->isTileSolid())
				continue;

			AABB2 tileBounds((float)tileX, (float)tileY, (float)tileX + 1.f, (float)tileY + 1.f);
			Vec2 nearestPoint = tileBounds.GetNearestPoint(entityPos);

			Vec2 displacement = entityPos - nearestPoint;
			float distanceToPlayer = displacement.GetLength();
			if (distanceToPlayer < 0.0001f)
			{
				displacement = Vec2(1.f, 0.f);
				distanceToPlayer = 0.0001f;
			}

			float overlap = entityRadius - distanceToPlayer;
			if (overlap > 0.f)
			{
				entityPos += (displacement / distanceToPlayer) * overlap;
			}
		}

		e->m_position = entityPos;
	}
}

void Map::DeleteGarbage()
{
	for (int i = 0; i < (int)m_allEntities.size(); i++)
	{
		if (m_allEntities[i]->m_isGarbage)
		{
			delete m_allEntities[i];
			m_allEntities.erase(m_allEntities.begin() + i);
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Map Initialization Helpers
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::PopulateTiles(TileType fill)
{
	for (int y = 0; y < m_dimensions.y; y++)
	{
		for (int x = 0; x < m_dimensions.x; x++)
		{
			m_tiles.emplace_back(IntVec2(x, y), fill);
		}
	}
}


void Map::SetBorderTiles(TileType border)
{
	for (int x = 0; x < m_dimensions.x; x++)
	{
		SetTileType(IntVec2(x, 0), border);
		SetTileType(IntVec2(x, m_dimensions.y - 1), border);
	}
	for (int y = 0; y < m_dimensions.y; y++)
	{
		SetTileType(IntVec2(0, y), border);
		SetTileType(IntVec2(m_dimensions.x - 1, y), border);
	}
}


void Map::PopulateSprinkleTypes( TileType sprinkleType1, TileType sprinkleType2)
{
	float sprinkle1Chance = 0.15f; 
	float sprinkle2Chance = 0.25f;

	for (int y = 1; y < m_dimensions.y - 1; y++)
	{
		for (int x = 1; x < m_dimensions.x - 1; x++)
		{
			float randomFloat = g_rng->RollRandomFloatZeroToOne();

			if (randomFloat < sprinkle1Chance)
			{
				SetTileType(IntVec2(x, y), sprinkleType1);
			}
			else if (randomFloat < sprinkle2Chance)
			{
				SetTileType(IntVec2(x, y), sprinkleType2);
			}
		}
	}
}

void Map::GenerateWorms(TileType wormTile, int numWorms, int wormLength)
{
	IntVec2 dims = m_dimensions;

	for (int w = 0; w < numWorms; w++)
	{
		int x = g_rng->RollRandomIntInRange(1, dims.x - 2);
		int y = g_rng->RollRandomIntInRange(1, dims.y - 2);

		SetTileType(IntVec2(x, y), wormTile);

		for (int step = 0; step < wormLength; step++)
		{
			int dir = g_rng->RollRandomIntInRange(0, 3);

			IntVec2 move = IntVec2(0,0);
			if (dir == 0) move = IntVec2(1, 0);   // East
			if (dir == 1) move = IntVec2(0, 1);   // North
			if (dir == 2) move = IntVec2(-1, 0);  // West
			if (dir == 3) move = IntVec2(0, -1);  // South

			IntVec2 next = IntVec2(x, y) + move;

			if (next.x <= 0 || next.y <= 0 ||
				next.x >= dims.x - 1 || next.y >= dims.y - 1)
			{
				continue;
			}

			x = next.x;
			y = next.y;

			SetTileType(IntVec2(x, y), wormTile);
		}
	}
}


void Map::CreateReservedAreas( TileType bunkerFloor, TileType bunkerWall)
{
	//Southwest Grass Area + Barrier
	for (int y = 1; y <= 5; y++)
	{
		for (int x = 1; x <= 5; x++)
		{
			SetTileType(IntVec2(x, y), bunkerFloor);
		}
	}

	SetTileType(IntVec2(2, 4), bunkerWall);
	SetTileType(IntVec2(3, 4), bunkerWall);
	SetTileType(IntVec2(4, 4), bunkerWall);
	SetTileType(IntVec2(4, 3), bunkerWall);
	SetTileType(IntVec2(4, 2), bunkerWall);

	SetTileType(IntVec2(1, 1), SPAWN_TILE);

	//Northeast Grass Area + Barrier
	int areaWidth = 6;
	int areaHeight = 6;

	int startX = m_dimensions.x - areaWidth;
	int startY = m_dimensions.y - areaHeight;

	for (int y = startY-1; y <= startY + areaHeight; y++)
	{
		for (int x = startX-1; x <= startX + areaWidth; x++)
		{
			SetTileType(IntVec2(x, y), bunkerFloor);
		}
	}

	SetTileType(IntVec2(startX + 0, startY), bunkerWall);
	SetTileType(IntVec2(startX + 1, startY), bunkerWall);
	SetTileType(IntVec2(startX + 2, startY), bunkerWall);
	SetTileType(IntVec2(startX + 3, startY), bunkerWall);
	SetTileType(IntVec2(startX + 0, startY + 1), bunkerWall);
	SetTileType(IntVec2(startX + 0, startY + 2), bunkerWall);
	SetTileType(IntVec2(startX + 0, startY + 3), bunkerWall);

	IntVec2 exitCoords(m_dimensions.x - 2, m_dimensions.y - 2);
	SetTileType(exitCoords, EXIT_TILE);
}

void Map::PopulateDijkstraMap(
	TileHeatMap& out_dijkstraMap,
	IntVec2 startCoords,
	float maxCost,
	[[maybe_unused]]bool treatWaterAsSolid
)
{
	out_dijkstraMap.SetAllValues(maxCost);

	// Validate start
	if (startCoords.x < 0 || startCoords.y < 0 ||
		startCoords.x >= m_dimensions.x ||
		startCoords.y >= m_dimensions.y)
	{
		return;
	}

	out_dijkstraMap.SetTileHeatValue(startCoords, 0.f);

	// Cardinal neighbor offsets
	static const IntVec2 cardinalOffsets[4] = {
		IntVec2(1,  0), // East
		IntVec2(-1,  0), // West
		IntVec2(0,  1), // North
		IntVec2(0, -1)  // South
	};

	bool didModify = true;

	while (didModify)
	{
		didModify = false;

		for (int y = 0; y < m_dimensions.y; y++)
		{
			for (int x = 0; x < m_dimensions.x; x++)
			{
				IntVec2 tileCoords(x, y);
				float currentCost = out_dijkstraMap.GetTileHeatValue(tileCoords);

				if (currentCost == maxCost)
					continue;

				for (int dirIndex = 0; dirIndex < 4; dirIndex++)
				{
					IntVec2 neighbor = tileCoords + cardinalOffsets[dirIndex];

					if (neighbor.x < 0 || neighbor.y < 0 ||
						neighbor.x >= m_dimensions.x ||
						neighbor.y >= m_dimensions.y)
					{
						continue;
					}

					if (IsTileSolid(neighbor))
					{
						continue;
					}

					float neighborCost = out_dijkstraMap.GetTileHeatValue(neighbor);
					float newCost = currentCost + 1.0f;

					// Only update if we found a cheaper path
					if (newCost < neighborCost)
					{
						out_dijkstraMap.SetTileHeatValue(neighbor, newCost);
						didModify = true;
					}
				}
			}
		}
	}
}

void Map::FillUnreachableTiles()
{
	if (!m_dijkstraMap)
		return;

	float unreachableCost = 999.f; // must match value used in PopulateDijkstraMap

	for (int y = 0; y < m_dimensions.y; y++)
	{
		for (int x = 0; x < m_dimensions.x; x++)
		{
			IntVec2 tileCoords(x, y);

			float heat = m_dijkstraMap->GetTileHeatValue(tileCoords);

			// Still unreachable
			if (heat >= unreachableCost)
			{
				// Avoid modifying border
				if (IsTileSolid(tileCoords))
					continue;

				// Set unreachable tiles into a type you want
				SetTileType(tileCoords, STONE); // choose whatever makes sense
			}
		}
	}
}

bool Map::IsExitReachable() const
{
	if (!m_dijkstraMap)
		return false;

	IntVec2 exitCoords((int)m_exitPosition.x, (int)m_exitPosition.y);

	float cost = m_dijkstraMap->GetTileHeatValue(exitCoords);

	return cost < 999.f;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Raycasting
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RaycastResult2D Map::RaycastVsTiles2D(Vec2 startPos, Vec2 fwdNormal, float maxDist)
{
	int numSteps = 1 + static_cast<int>(maxDist * 10.f);
	Vec2 currentPos = startPos;
	float tinyStepLength = maxDist / static_cast<float>(numSteps);
	Vec2 tinyStepForward = fwdNormal * tinyStepLength;
	for (int i = 0; i < numSteps; ++i)
	{
		currentPos += tinyStepForward;
		if (IsPointInSolid(currentPos))
		{
			RaycastResult2D hitResult;
			hitResult.m_didImpact = true;
			hitResult.m_impactPos = currentPos;
			//hitResult.m_impactNormal = ? ? ? ;
			hitResult.m_impactDist = GetDistance2D(startPos, currentPos);
			return hitResult;
		}
	}

	RaycastResult2D missResult;
	missResult.m_didImpact = false;
	missResult.m_impactPos = startPos + (fwdNormal * maxDist);
	//missResult.m_impactNormal = ? ? ? ;
	missResult.m_impactDist = maxDist;
	return missResult;
}


bool Map::HasLineOfSight(Vec2 const& startPos, Vec2 const& endPos)
{
	Vec2 displacement = endPos - startPos;
	float distance = displacement.GetLength();
	Vec2 direction = displacement.GetNormalized();

	RaycastResult2D raycast = RaycastVsTiles2D(startPos, direction, distance);

	if (raycast.m_didImpact && raycast.m_impactDist < distance)
	{
		return false;
	}

	return true;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Tile Functions
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Tile* Map::GetTileAtTileCoords(const IntVec2& tileCoords)
{
	if (tileCoords.x < 0 || tileCoords.y < 0 || tileCoords.x >= m_dimensions.x || tileCoords.y >= m_dimensions.y)
	{
		return nullptr;
	}

	int index = tileCoords.y * m_dimensions.x + tileCoords.x;
	return &m_tiles[index];
}


IntVec2 Map::GetTileCoordsForWorldPos(Vec2 const& worldPos) const
{
	return IntVec2((int)floorf(worldPos.x), (int)floorf(worldPos.y));
}


void Map::SetTileType(const IntVec2& tileCoords, TileType type)
{
	Tile* tile = GetTileAtTileCoords(tileCoords);
	if (tile)
	{
		tile->m_type = type;
	}
}


bool Map::IsTileSolid(IntVec2 const& tileCoords) const
{
	if (tileCoords.x < 0 || tileCoords.y < 0 || tileCoords.x >= m_dimensions.x || tileCoords.y >= m_dimensions.y)
		return true;

	const Tile& tile = m_tiles[tileCoords.y * m_dimensions.x + tileCoords.x];
	return tile.isTileSolid();
}


bool Map::IsPointInSolid(Vec2 const& point) const
{
	IntVec2 tileCoords((int)floorf(point.x), (int)floorf(point.y));
	return IsTileSolid(tileCoords);
}


//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Spawning
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Entity* Map::SpawnNewEntity(EntityType type, Vec2 const& position, float orientationDegrees /*= 0.f*/)
{
	Entity* newEntity = nullptr;

	switch (type)
	{
	case ENTITY_TYPE_GOOD_PLAYER:
		newEntity = new Player(m_game, position);
		newEntity->m_faction = FACTION_GOOD;
		m_player = dynamic_cast<Player*>(newEntity);
		break;

	case ENTITY_TYPE_EVIL_SCORPIO:
		newEntity = new Scorpio(m_game, position);
		newEntity->m_faction = FACTION_EVIL;
		break;

	case ENTITY_TYPE_EVIL_LEO:
		newEntity = new Leo(m_game, position);
		newEntity->m_faction = FACTION_EVIL;
		break;

	case ENTITY_TYPE_EVIL_ARIES:
		newEntity = new Aries(m_game, position);
		newEntity->m_faction = FACTION_EVIL;
		break;

	case ENTITY_TYPE_GOOD_BULLET:
		newEntity = new Bullet(m_game, position, Vec2::MakeFromPolarDegrees(orientationDegrees, 2.f), FACTION_GOOD);
		break;

	case ENTITY_TYPE_EVIL_BULLET:
		newEntity = new Bullet(m_game, position, Vec2::MakeFromPolarDegrees(orientationDegrees, 1.f), FACTION_EVIL);
		break;

	case ENTITY_TYPE_EVIL_BOLT:
		newEntity = new Bullet(m_game, position, Vec2::MakeFromPolarDegrees(orientationDegrees, 3.f), FACTION_EVIL);
		break;

	case ENTITY_TYPE_EXPLOSION:
		newEntity = new Explosion(m_game, position, orientationDegrees); //Bad Practice, but better than making explosion have its own spawn function???
		break;															 //orientationDegrees is used to set the size of the explosion

		// 	default:
		// 		ERROR_AND_DIE("SpawnNewEntity called with invalid type!");
	}

	if (newEntity)
	{
		newEntity->m_type = type;
		AddEntityToMap(*newEntity);
	}

	return newEntity;
}

void Map::AddEntityToMap(Entity& entity)
{
	m_allEntities.push_back(&entity);
	m_entityListsByType[entity.m_type].push_back(&entity);
}

void Map::RemoveEntityFromMap(Entity& entity)
{
	for (int i = 0; i < (int)m_allEntities.size(); i++)
	{
		if (m_allEntities[i] == &entity)
		{
			m_allEntities.erase(m_allEntities.begin() + i);
			break;
		}
	}

	EntityList& list = m_entityListsByType[entity.m_type];
	for (int i = 0; i < (int)list.size(); i++)
	{
		if (list[i] == &entity)
		{
			list.erase(list.begin() + i);
			break;
		}
	}
}

