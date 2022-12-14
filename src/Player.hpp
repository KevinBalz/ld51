#pragma once
#include <Input.hpp>
#include <World.hpp>
#include "Comps.hpp"
#include "Entity.hpp"
#include "FrameData.hpp"
#include "Jam/TileMap.hpp"
#include "SmallVec.hpp"
#include "Audio.hpp"

constexpr const ClipData PlayerIdleClip{0, 1, 0.4f};

inline void PlayerUpdate(SharedData* sharedData, FrameData* frameData, tako::Input* input, float dt, tako::World& world, int tileMap)
{
	world.IterateComps<Player, Position, RigidBody, Animator, SpriteRenderer>([&](Player& player, Position& pos, RigidBody& body, Animator& animator, SpriteRenderer& renderer)
	{
		constexpr float speed = 50;
		constexpr auto acceleration = 0.2f;
		float moveX = input->GetAxis(tako::Axis::Left).x;
		if (std::abs(moveX) < 0.1f)
		{
			moveX = 0;
		}
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
			if (grounded)
			{
				sharedData->audio->Play("/Jump.wav");
			}
		}

		body.velocity.x = moveX = acceleration * moveX + (1 - acceleration) * body.velocity.x;
		animator.flipX = body.velocity.x == 0 ? animator.flipX : body.velocity.x < 0;

		player.usedDashes = grounded ? 0 : player.usedDashes;
		player.dashCooldown -= dt;
		if (player.unlocked[0] && player.dashCooldown <= 0 && player.usedDashes < 1 && (input->GetKeyDown(tako::Key::C) || input->GetKeyDown(tako::Key::Gamepad_X) || input->GetKeyDown(tako::Key::Gamepad_R2) || input ->GetKeyDown(tako::Key::Gamepad_R)))
		{
			body.velocity.x = tako::mathf::sign(moveX) * 750;
			body.velocity.y = 0;
			player.dashCooldown = 1;
			player.usedDashes++;
			sharedData->audio->Play("/Dash.wav");
		}

		auto absVel = std::abs(body.velocity.x);
		if (absVel > speed * 2)
		{
			auto dashRen = renderer;
			dashRen.alpha = 128;
			Position dashPos = pos;
			world.Create
			(
				std::move(dashPos),
				std::move(dashRen),
				FadeOut{100, 0.5f}
			);
			animator.PlayClip({6, 6, 1337});
		}
		else if (!grounded)
		{
			animator.PlayClip({7, 8, 0.15f});
		}
		else if (absVel > 1)
		{
			animator.PlayClip({2, 5, 0.15f});
			player.stepCounter += dt;
			if (player.stepCounter > 0.3f)
			{
				sharedData->audio->Play("/Step.wav");
				player.stepCounter = 0;
			}
		}
		else
		{
			animator.PlayClip(PlayerIdleClip);
		}
		body.velocity.y -= dt * 200;
		body.velocity.y = std::max(body.velocity.y, -400.0f);

		static float prevAxisY = 0;
		auto axisY = input->GetAxis(tako::Axis::Left).y;
		auto axisUpDown = axisY > 0.9f && prevAxisY < 0.9f;
		prevAxisY = axisY;

		auto playerRec = body.CalcRec(pos.position);
		if (axisUpDown || input->GetKeyDown(tako::Key::Up) || input->GetKeyDown(tako::Key::W) || input->GetKeyDown(tako::Key::X) || input->GetKeyDown(tako::Key::Gamepad_Dpad_Up) || input->GetKeyDown(tako::Key::Gamepad_B))
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
					sharedData->audio->Play("/Activate.wav");
				}
			});
		}

		if (player.unlocked[2] && input->GetKeyDown(tako::Key::B))
		{
			player.clockMode = player.clockMode != ClockMode::Binary ? ClockMode::Binary : player.unlocked[1] ? ClockMode::Hexa : ClockMode::Decimal;
		}

		static auto prevGrounded = grounded;
		if (!prevGrounded && grounded)
		{
			sharedData->audio->Play("/Land.wav");
		}
		prevGrounded = grounded;

		frameData->collectedCount = 0;
		for (int i = 0; i < player.collected.size(); i++)
		{
			if (player.collected[i])
			{
				frameData->collectedCount++;
			}
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
				if (up.upgradeID > 0)
				{
					player.clockMode = static_cast<ClockMode>(up.upgradeID);
				}
				std::string str;
				switch (up.upgradeID)
				{
					case 0:
						str = "Dash unlocked! \nPress [C] while\nmoving to dash";
						break;
					case 1:
						str = "Found Hexadecimal Clock! \nThe clock now has a\nduration of 10 in base 16";
						break;
					case 2:
						str = "Found Binary Clock! \nYou can toggle the\nclock to use base 2\nby pressing [B]";
						break;

				}
				sharedData->ShowText(str, true);
				sharedData->audio->Play("/Upgrade.wav");
			}
		});

		world.IterateComps<tako::Entity, Position, Collectible>([&](tako::Entity entity, Position& sPos, Collectible& col)
		{
			tako::Jam::PlatformerPhysics2D::Rect sRec(sPos.position, {16, 16});
			if (player.collected[col.id])
			{
				toDelete.Push(entity);
				return;
			}
			if (tako::Jam::PlatformerPhysics2D::Rect::Overlap(playerRec, sRec))
			{
				player.collected[col.id] = true;
				frameData->collectedCount++;
				toDelete.Push(entity);
				if (frameData->collectedCount < player.collected.size())
				{
					sharedData->ShowText(fmt::format("Found {} of {}", frameData->collectedCount, player.collected.size()));
					sharedData->audio->Play("/Collect.wav");
				}
				else
				{
					sharedData->ShowText(fmt::format("Congratulations!\n You found all\n{} orbs!\nThank you for\nplaying my game!", player.collected.size()), true);
					sharedData->audio->Play("/Collect.wav");
				}
			}
		});
		for (int i = 0; i < toDelete.GetLength(); i++)
		{
			world.Delete(toDelete[i]);
		}
	});

}
