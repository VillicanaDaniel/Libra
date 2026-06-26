#pragma once
#include "Engine/Core/EngineCommon.hpp"
#include "Game/App.hpp"
struct Vec2;
struct Rgba8;

extern float WORLD_SIZE_X;
extern float WORLD_SIZE_Y;

extern float WORLD_CENTER_X;
extern float WORLD_CENTER_Y;

extern float SCREEN_SIZE_X;
extern float SCREEN_SIZE_Y;

void DebugDrawRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color);
void DebugDrawLine(Vec2 const& center, Vec2 end, float thickness, Rgba8 const& color);