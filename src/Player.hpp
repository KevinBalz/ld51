#pragma once
#include <Input.hpp>
#include <World.hpp>
#include "Comps.hpp"
#include "FrameData.hpp"
#include "Jam/TileMap.hpp"


inline void PlayerUpdate(FrameData* frameData, tako::Input* input, float dt, tako::World& world, int tileMap)
{
	world.IterateComps<Player, Position, RigidBody>([&](Player& player, Position& pos, RigidBody& body)
	{
		constexpr float speed = 50;
		constexpr auto acceleration = 0.2f;
		float moveX = input->GetAxis(tako::Axis::Left).x;
		if (input->GetKey(tako::Key::Left) || input->GetKey(tako::Key::A) || input->GetKey(tako::Key::Gamepad_Dpad_Left))
		{
			moveX -= 1;
		}
		if (input->GetKey(tako::Key::Right) || input->GetKey(tako::Key::D) || input->GetKey(tako::Key::Gamepad_Dpad_Right))
		{
			moveX += 1;
		}

		moveX = tako::mathf::clamp(moveX, -1, 1);
		moveX *= speed;

		auto grounded = player.grounded;
		player.airTime = grounded ? 0 : player.airTime + dt;
		if (player.airTime < 0.3f && (input->GetKey(tako::Key::Space) || input->GetKey(tako::Key::Gamepad_A)))
		{
			body.velocity.y = 80;
		}


		body.velocity.x = moveX = acceleration * moveX + (1 - acceleration) * body.velocity.x;
		body.velocity.y -= dt * 200;

		if (input->GetKey(tako::Key::Up) || input->GetKey(tako::Key::W) || input->GetKey(tako::Key::Gamepad_Dpad_Up))
		{
			auto playerRec = body.CalcRec(pos.position);
			world.IterateComps<Position, PlayerSpawn>([&](Position& sPos, PlayerSpawn& spawn)
			{
				if (frameData->triggeredCheckpoint) return;
				tako::Jam::PlatformerPhysics2D::Rect sRec(sPos.position, {16, 16});
				if (tako::Jam::PlatformerPhysics2D::Rect::Overlap(playerRec, sRec))
				{
					player.spawnID = spawn.id;
					player.spawnMap = tileMap;
					frameData->triggeredCheckpoint = true;
				}
			});
		}
	});

}
