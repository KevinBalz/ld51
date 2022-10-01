#pragma once
#include <Tako.hpp>
#include <OpenGLPixelArtDrawer.hpp>
#include <World.hpp>
#include <PlatformerPhysics2D.hpp>
#include <Jam/LDtkImporter.hpp>
#include "Comps.hpp"
#include "Entity.hpp"
#include <Font.hpp>
#include "FrameData.hpp"
#include "Jam/TileMap.hpp"
#include "Player.hpp"
#include "Reflection.hpp"


template <typename T>
void AssignLDtkField(const char* typeName, void* data, nlohmann::json& json, const tako::Reflection::StructInformation::Field& info)
{
	for (auto& field : json)
	{
		if (field["__type"] == typeName)
		{
			*reinterpret_cast<T*>(reinterpret_cast<tako::U8*>(data) + info.offset) = field["__value"];
		}
	}
}

void ApplyLDtkFields(void* data, nlohmann::json& json, const tako::Reflection::StructInformation* structType)
{
	for (auto& info : structType->fields)
	{
		if (tako::Reflection::GetPrimitiveInformation<int>() == info.type)
		{
			AssignLDtkField<int>("Int", data, json, info);
		}
		else if (tako::Reflection::GetPrimitiveInformation<bool>() == info.type)
		{
			AssignLDtkField<bool>("Bool", data, json, info);
		}
	}
}


template<typename T>
tako::Entity SpawnTileEntity(tako::World& world, tako::Jam::TileEntity& entDef)
{
	auto ent = world.Create
	(
		Position{{entDef.position}}
	);


	world.AddComponent<T>(ent);
	auto& comp = world.GetComponent<T>(ent);
	new (&comp) T();
	ApplyLDtkFields(&comp, entDef.fields, tako::Reflection::Resolver::Get<T>());
	return ent;
}

inline tako::Texture CreateText(tako::OpenGLPixelArtDrawer* drawer, tako::Font* font, std::string_view text)
{
	auto bitmap = font->RenderText(text, 1);
    return drawer->CreateTexture(bitmap);
}

inline void UpdateText(tako::OpenGLPixelArtDrawer* drawer, tako::Font* font, std::string_view text, tako::Texture& tex)
{
	auto bitmap = font->RenderText(text, 1);
    drawer->UpdateTexture(tex, bitmap);
}


class Game
{
public:
	template<typename T>
	void RegisterTileEntity()
	{
		auto info = tako::Reflection::Resolver::Get<T>();
		m_entityInstantiate[info->name] = &SpawnTileEntity<T>;
	}

	template<typename T, typename Cb>
	void RegisterTileEntity(Cb&& callback)
	{
		auto info = tako::Reflection::Resolver::Get<T>();
		m_entityInstantiate[info->name] = [=](tako::World& world, tako::Jam::TileEntity& entDef)
		{
			auto ent = SpawnTileEntity<T>(world, entDef);
			callback(world, ent, entDef);
			return ent;
		};
	}

	Game()
	{
		RegisterTileEntity<PlayerSpawn>([](auto& world, auto ent, auto& entDef)
		{
			LOG("It worked!");
		});
	}

	void LoadLevel(int id)
	{
		m_world.Reset();
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

		for (auto& entDef : level.entities)
		{
			m_entityInstantiate[entDef.typeName](m_world, entDef);
		}

		tako::Vector2 spawnPos;
		m_world.IterateComps<Position, PlayerSpawn>([&](Position& pos, PlayerSpawn& spawn)
		{
			if (spawn.id == 0)
			{
				spawnPos = pos.position;
			}
		});

		m_world.Create
		(
			Player(),
			Position{spawnPos},
			RigidBody{{0, 0}, {0, 0, 12, 16}},
			RectRenderer{{16, 16}, {255, 0, 0, 255}},
			Camera()
		);
		ResetWorldClock();
	}

	void ResetWorldClock()
	{
		m_worldClock = 10;
	}

	void UpdateClockText()
	{
		int num = std::ceil(m_worldClock);

		char firstDigit;
		if (num >= 10)
		{
			firstDigit = num / 10 + '0';
		}
		else
		{
			firstDigit = '0';
		}
		char secondDigit;
		secondDigit = num % 10 + '0';
		std::stringstream stream;
		stream << firstDigit << secondDigit;
		auto str = stream.str();
		if (str != m_renderedClockText)
		{
			UpdateText(drawer, m_font, str, m_clockTex);
			m_renderedClockText = str;
		}
	}

	void Setup(const tako::SetupData& setup)
	{
		drawer = new tako::OpenGLPixelArtDrawer(setup.context);
		context = setup.context;
		drawer->SetTargetSize(240, 135);
		drawer->AutoScale();

		m_font = new tako::Font("/charmap-cellphone.png", 5, 7, 1, 1, 2, 2,
			" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]\a_`abcdefghijklmnopqrstuvwxyz{|}~");
		m_clockTex = CreateText(drawer, m_font, m_renderedClockText);

		m_tileWorld = tako::Jam::LDtkImporter::LoadWorld("/World.ldtk");
		LoadLevel(0);
	}


	void Update(const tako::GameStageData stageData, tako::Input* input, float dt)
	{
		auto frameData = reinterpret_cast<FrameData*>(stageData.frameData);
		PlayerUpdate(frameData, input, dt, m_world);

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
		tako::Jam::PlatformerPhysics2D::SimulatePhysics(m_nodesCache, {m_activeLevel->collision, {16, 16}, (int) m_activeLevel->size.x / 16, (int) m_activeLevel->size.y / 16 }, [](auto& self, auto& other) { LOG("col!");});

		m_worldClock -= dt;
		if (frameData->triggeredCheckpoint)
		{
			ResetWorldClock();
		}
		if (m_worldClock <= 0)
		{
			ResetWorldClock();
			m_world.IterateComps<Position, PlayerSpawn>([&](Position& pos, PlayerSpawn& spawn)
			{
				m_world.IterateComps<Position, Player>([&](Position& pPos, Player& player)
				{
					if (player.spawnID == spawn.id)
					{
						pPos.position = pos.position;
					}
				});
			});
		}

		m_world.IterateComps<Position, Camera>([&](Position& pos, Camera& cam)
		{
			drawer->SetCameraPosition(pos.position);
		});
		UpdateClockText();
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

		//auto m_cameraSize = drawer->GetCameraViewSize();
		//drawer->SetCameraPosition(m_cameraSize/2);
		drawer->SetCameraPosition({0 , 0});
		drawer->DrawImage((float) -m_clockTex.width / 2, 42, m_clockTex.width, m_clockTex.height, m_clockTex.handle);
	}

private:
	tako::OpenGLPixelArtDrawer* drawer;
	tako::GraphicsContext* context;
	tako::World m_world;
	tako::Jam::TileWorld m_tileWorld;
	tako::Jam::TileMap* m_activeLevel;
	float m_worldClock;
	std::vector<tako::Texture> m_layerCache;
	std::vector<tako::Jam::PlatformerPhysics2D::Node> m_nodesCache;
	std::map<std::string, std::function<tako::Entity(tako::World&, tako::Jam::TileEntity&)>> m_entityInstantiate;

	tako::Font* m_font;
	std::string m_renderedClockText = "10";
	tako::Texture m_clockTex;
};
