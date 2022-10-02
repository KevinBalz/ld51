#pragma once
#include <Input.hpp>
#include <World.hpp>
#include "Comps.hpp"
#include "Entity.hpp"
#include "FrameData.hpp"
#include "Jam/TileMap.hpp"
#include "SmallVec.hpp"


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

		player.usedDashes = grounded ? 0 : player.usedDashes;
		player.dashCooldown -= dt;
		if (player.unlocked[0] && player.dashCooldown <= 0 && player.usedDashes < 1 && input->GetKeyDown(tako::Key::C))
		{
			body.velocity.x = tako::mathf::sign(moveX) * 750;
			body.velocity.y = 0;
			player.dashCooldown = 1;
			player.usedDashes++;
		}
		body.velocity.y -= dt * 200;

		static float prevAxisY = 0;
		auto axisY = input->GetAxis(tako::Axis::Left).y;
		auto axisUpDown = axisY > 0.9f && prevAxisY < 0.9f;
		prevAxisY = axisY;

		auto playerRec = body.CalcRec(pos.position);
		if (axisUpDown || input->GetKeyDown(tako::Key::Up) || input->GetKeyDown(tako::Key::W) || input->GetKeyDown(tako::Key::Gamepad_Dpad_Up) || input->GetKeyDown(tako::Key::Gamepad_B))
		{
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

		tako::SmallVec<tako::Entity, 4> toDelete;
		world.IterateComps<tako::Entity, Position, Upgrade>([&](tako::Entity entity, Position& sPos, Upgrade& up)
		{
			tako::Jam::PlatformerPhysics2D::Rect sRec(sPos.position, {16, 16});
			if (player.unlocked[up.upgradeID])
			{
				toDelete.Push(entity);
				return;
			}
			if (tako::Jam::PlatformerPhysics2D::Rect::Overlap(playerRec, sRec))
			{
				player.unlocked[up.upgradeID] = true;
				toDelete.Push(entity);
			}
		});
		for (int i = 0; i < toDelete.GetLength(); i++)
		{
			world.Delete(toDelete[i]);
		}
	});

}
