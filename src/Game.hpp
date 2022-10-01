#pragma once
#include <Tako.hpp>
#include <OpenGLPixelArtDrawer.hpp>
#include <World.hpp>
#include "PlatformerPhysics2D.hpp"
#include "Comps.hpp"

class Game
{
public:
	Game()
	{}

	void Setup(const tako::SetupData& setup)
	{
		drawer = new tako::OpenGLPixelArtDrawer(setup.context);
		context = setup.context;
		drawer->SetTargetSize(240, 135);
		drawer->AutoScale();

		m_world.Create
		(
			Player(),
			Position{{0, 0}},
			RigidBody{{0, 0}, {0, 0, 16, 16}},
			RectRenderer{{16, 16}, {255, 0, 0, 255}}
		);
	}


	void Update(const tako::GameStageData stageData, tako::Input* input, float dt)
	{
		m_world.IterateComps<Player, Position, RigidBody>([&](Player& player, Position& pos, RigidBody& body)
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
			body.velocity.x = moveX = acceleration * moveX + (1 - acceleration) * body.velocity.x;
		});


		m_nodesCache.clear();
		m_world.IterateComps<tako::Entity, Position, RigidBody>([&](tako::Entity entity, Position& pos, RigidBody& body)
		{
			m_nodesCache.push_back
			({
				pos.position,
				body.velocity,
				body.bounds,
				nullptr,
				{0, 0}
			});
		});
		tako::Jam::PlatformerPhysics2D::CalculateMovement(dt, m_nodesCache);
		tako::Jam::PlatformerPhysics2D::SimulatePhysics(m_nodesCache, [](auto& self, auto& other) { LOG("col!");});
	}

	void Draw(const tako::GameStageData stageData)
	{
		drawer->Resize(context->GetWidth(), context->GetHeight());
		drawer->Clear();

		m_world.IterateComps<Position, RectRenderer>([&](Position& pos, RectRenderer& ren)
		{
			drawer->DrawRectangle(pos.position.x - ren.size.x / 2, pos.position.y + ren.size.y / 2, ren.size.x, ren.size.y, ren.color);
		});
	}

private:
	tako::OpenGLPixelArtDrawer* drawer;
	tako::GraphicsContext* context;
	tako::World m_world;
	std::vector<tako::Jam::PlatformerPhysics2D::Node> m_nodesCache;
};
