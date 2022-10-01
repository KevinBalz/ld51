#pragma once
#include <Reflection.hpp>
#include <Math.hpp>
#include <World.hpp>
#include "PlatformerPhysics2D.hpp"

struct Position
{
	tako::Vector2 position;
};

struct RectRenderer
{
	tako::Vector2 size;
	tako::Color color;
};

struct RigidBody
{
	tako::Vector2 velocity;
	tako::Jam::PlatformerPhysics2D::Rect bounds;

	tako::Jam::PlatformerPhysics2D::Rect CalcRec(tako::Vector2 position)
	{
		return {position.x + bounds.x, position.y + bounds.y, bounds.w, bounds.h};
	}
};

struct Player
{
	int spawnID = 0;
	int spawnMap = 0;
	float airTime = 0;
};

struct Camera
{
};

struct PlayerSpawn
{
	int id;
	REFLECT()
};
