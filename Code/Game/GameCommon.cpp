#include "Game/GameCommon.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"

float WORLD_SIZE_X;
float WORLD_SIZE_Y;

float WORLD_CENTER_X;
float WORLD_CENTER_Y;

float SCREEN_SIZE_X;
float SCREEN_SIZE_Y;

void DebugDrawRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color)
{
	float halfThickness = 0.5f * thickness;
	float innerRadius = radius - halfThickness;
	float outerRadius = radius + halfThickness;
	constexpr int NUM_SIDES = 36;
	constexpr int NUM_TRIS = 2 * NUM_SIDES;
	constexpr int NUM_VERTS = 3 * NUM_TRIS;
	Vertex verts[NUM_VERTS];
	constexpr float DEGREES_PER_SIDE = 360.f / static_cast<float>(NUM_SIDES);
	for (int sideNum = 0; sideNum < NUM_SIDES; ++sideNum)
	{
		float startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum);
		float endDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum + 1);
		float cosStart = CosDegrees(startDegrees);
		float sinStart = SinDegrees(startDegrees);
		float cosEnd = CosDegrees(endDegrees);
		float sinEnd = SinDegrees(endDegrees);

		Vec3 innerStartPos = Vec3(center.x + innerRadius * cosStart, center.y + innerRadius * sinStart, 0.f);
		Vec3 outerStartPos = Vec3(center.x + outerRadius * cosStart, center.y + outerRadius * sinStart, 0.f);
		Vec3 innerEndPos = Vec3(center.x + innerRadius * cosEnd, center.y + innerRadius * sinEnd, 0.f);
		Vec3 outerEndPos = Vec3(center.x + outerRadius * cosEnd, center.y + outerRadius * sinEnd, 0.f);

		int vertIndexA = (6 * sideNum) + 0;
		int vertIndexB = (6 * sideNum) + 1;
		int vertIndexC = (6 * sideNum) + 2;
		int vertIndexD = (6 * sideNum) + 3;
		int vertIndexE = (6 * sideNum) + 4;
		int vertIndexF = (6 * sideNum) + 5;

		verts[vertIndexA].m_position = innerEndPos;
		verts[vertIndexB].m_position = innerStartPos;
		verts[vertIndexC].m_position = outerStartPos;
		verts[vertIndexA].m_color = color;
		verts[vertIndexB].m_color = color;
		verts[vertIndexC].m_color = color;

		verts[vertIndexD].m_position = innerEndPos;
		verts[vertIndexE].m_position = outerStartPos;
		verts[vertIndexF].m_position = outerEndPos;
		verts[vertIndexD].m_color = color;
		verts[vertIndexE].m_color = color;
		verts[vertIndexF].m_color = color;
	}

	g_engine->m_render->DrawVertexArray(NUM_VERTS, verts);
}

void DebugDrawLine(Vec2 const& start, Vec2 end, float thickness, Rgba8 const& color)
{
	Vec2 direction = end - start;
	Vec2 right = direction.GetNormalized();
	right = right.GetRotatedBy90Degrees();// Perpendicular to the line

	Vec2 offset = right * (thickness * 0.5f);

	Vec2 position0 = start + offset;
	Vec2 position1 = start - offset;
	Vec2 position2 = end + offset;
	Vec2 position3 = end - offset;

	Vertex verts[6];

	verts[0] = Vertex(Vec3(position0.x, position0.y, 0.f), color, Vec2(0.f, 0.f));
	verts[1] = Vertex(Vec3(position1.x, position1.y, 0.f), color, Vec2(0.f, 0.f));
	verts[2] = Vertex(Vec3(position2.x, position2.y, 0.f), color, Vec2(0.f, 0.f));

	verts[3] = Vertex(Vec3(position2.x, position2.y, 0.f), color, Vec2(0.f, 0.f));
	verts[4] = Vertex(Vec3(position1.x, position1.y, 0.f), color, Vec2(0.f, 0.f));
	verts[5] = Vertex(Vec3(position3.x, position3.y, 0.f), color, Vec2(0.f, 0.f));

	g_engine->m_render->DrawVertexArray(6, verts);
}