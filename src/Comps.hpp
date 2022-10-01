#pragma once
#include <Reflection.hpp>
#include <Math.hpp>
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
};

struct Player
{
};
