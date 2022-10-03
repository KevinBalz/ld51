#pragma once
#include <Reflection.hpp>
#include <Math.hpp>
#include <World.hpp>
#include <variant>
#include "PlatformerPhysics2D.hpp"
#include "Texture.hpp"
#include "OpenGLSprite.hpp"

using Rect = tako::Jam::PlatformerPhysics2D::Rect;

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
	std::variant<tako::Texture, tako::OpenGLSprite*> sprite;
	tako::Vector2 offset = {0, 0};
	tako::U8 alpha = 255;
};

struct RigidBody
{
	tako::Vector2 velocity;
	Rect bounds;

	Rect CalcRec(tako::Vector2 position)
	{
		return {position.x + bounds.x, position.y + bounds.y, bounds.w, bounds.h};
	}
};

enum class ClockMode
{
	Decimal = 0,
	Hexa = 1,
	Binary = 2
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
	ClockMode clockMode = ClockMode::Decimal;
	std::array<bool, 3> unlocked{false};
	std::array<bool, 8> collected{false};
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

struct Collectible
{
	int id;
	REFLECT()
};

struct AnimationData
{
	std::vector<tako::OpenGLSprite*> sprites;
	std::vector<tako::OpenGLSprite*> reverse;
};

struct Animator
{
	AnimationData* data;
	bool flipX = false;
};

struct FadeOut
{
	tako::U8 startFade;
	float duration;
	float left = duration;
};
