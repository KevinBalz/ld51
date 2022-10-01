#pragma once
#include <Input.hpp>
#include <World.hpp>
#include "Comps.hpp"


inline void PlayerUpdate(tako::Input* input, float dt, tako::World& world)
{
	world.IterateComps<Player, Position, RigidBody>([&](Player& player, Position& pos, RigidBody& body)
	{
		constexpr float speed = 20;
		constexpr auto acceleration = 0.2f;
		float moveX = 0;
		if (input->GetKey(tako::Key::Left) || input->GetKey(tako::Key::A) || input->GetKey(tako::Key::Gamepad_Dpad_Left))
		{
			moveX -= speed;
		}
		if (input->GetKey(tako::Key::Right) || input->GetKey(tako::Key::D) || input->GetKey(tako::Key::Gamepad_Dpad_Right))
		{
			moveX += speed;
		}


		auto grounded = body.velocity.y == 0;
		player.airTime = grounded ? 0 : player.airTime + dt;
		if (player.airTime < 0.3f && (input->GetKey(tako::Key::Up) || input->GetKey(tako::Key::W) || input->GetKey(tako::Key::Space) || input->GetKey(tako::Key::Gamepad_A)))
		{
			body.velocity.y = 80;
		}

		body.velocity.x = moveX = acceleration * moveX + (1 - acceleration) * body.velocity.x;
		body.velocity.y -= dt * 200;

		auto playerRec = body.CalcRec(pos.position);
		world.IterateComps<Position, PlayerSpawn>([&](Position& sPos, PlayerSpawn& spawn)
		{
			tako::Jam::PlatformerPhysics2D::Rect sRec(sPos.position, {16, 16});
			if (tako::Jam::PlatformerPhysics2D::Rect::Overlap(playerRec, sRec))
			{
				player.spawnID = spawn.id;
			}
		});
	});

}
