#pragma once
#include <Reflection.hpp>
#include <Math.hpp>
#include <World.hpp>
#include "PlatformerPhysics2D.hpp"
#include "Texture.hpp"

struct Position
{
	tako::Vector2 position;
};

struct RectRenderer
{
	tako::Vector2 size;
	tako::Color color;
};

struct SpriteRenderer
{
	tako::Texture sprite;
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
	float prevYVelocity = 0;
	bool grounded = false;
	float dashCooldown = 0.0f;
	float usedDashes = 0;
	std::array<bool, 4> unlocked;
};

struct Camera
{
	bool snapped = false;
	tako::Vector2 position;
};

struct PlayerSpawn
{
	int id;
	REFLECT()
};

struct Upgrade
{
	int upgradeID;
	REFLECT()
};
