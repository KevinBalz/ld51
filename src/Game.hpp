#pragma once
#include <Tako.hpp>
#include <OpenGLPixelArtDrawer.hpp>
#include <World.hpp>
#include <PlatformerPhysics2D.hpp>
#include <Jam/LDtkImporter.hpp>
#include "Comps.hpp"
#include "Jam/TileMap.hpp"

class Game
{
public:
	Game()
	{}


	void LoadLevel(int id)
	{
		auto& level = m_tileWorld.levels[id];
		m_activeLevel = &level;
		for (int i = 0; i < level.tileLayers.size(); i++)
		{
			auto& layer = level.tileLayers[i];
			if (i >= m_layerCache.size())
			{
				auto tex = drawer->CreateTexture(layer.composite);
				m_layerCache.push_back(tex);
			}
			else
			{
				drawer->UpdateTexture(m_layerCache[i], layer.composite);
			}
		}
	}

	void Setup(const tako::SetupData& setup)
	{
		drawer = new tako::OpenGLPixelArtDrawer(setup.context);
		context = setup.context;
		drawer->SetTargetSize(240, 135);
		drawer->AutoScale();

		m_tileWorld = tako::Jam::LDtkImporter::LoadWorld("/World.ldtk");
		LoadLevel(0);

		m_world.Create
		(
			Player(),
			Position{m_activeLevel->entities[0].position},
			RigidBody{{0, 0}, {0, 0, 12, 16}},
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
			body.velocity.y -= dt * 200;
			drawer->SetCameraPosition(pos.position);
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
		tako::Jam::PlatformerPhysics2D::SimulatePhysics(m_nodesCache, {m_activeLevel->collision, {16, 16}, 16, 16 }, [](auto& self, auto& other) { LOG("col!");});
	}


	void DrawEntities()
	{
		m_world.IterateComps<Position, RectRenderer>([&](Position& pos, RectRenderer& ren)
		{
			drawer->DrawRectangle(pos.position.x - ren.size.x / 2, pos.position.y + ren.size.y / 2, ren.size.x, ren.size.y, ren.color);
		});
	}

	void DrawTileLayer(int i)
	{
		auto& tex = m_layerCache[i];
		drawer->DrawImage(0, tex.height, tex.width, tex.height, tex.handle);
	}

	void Draw(const tako::GameStageData stageData)
	{
		drawer->Resize(context->GetWidth(), context->GetHeight());
		drawer->SetClearColor(m_activeLevel->backgroundColor);
		drawer->Clear();

		if (m_activeLevel->entityLayerIndex < 0)
		{
			DrawEntities();
		}
		for (int i = 0; i < m_activeLevel->tileLayers.size(); i++)
		{
			DrawTileLayer(i);
			if (m_activeLevel->entityLayerIndex == i)
			{
				DrawEntities();
			}
		}
	}

private:
	tako::OpenGLPixelArtDrawer* drawer;
	tako::GraphicsContext* context;
	tako::World m_world;
	tako::Jam::TileWorld m_tileWorld;
	tako::Jam::TileMap* m_activeLevel;
	std::vector<tako::Texture> m_layerCache;
	std::vector<tako::Jam::PlatformerPhysics2D::Node> m_nodesCache;
};
