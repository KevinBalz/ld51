#pragma once
#include <Tako.hpp>
#include <OpenGLPixelArtDrawer.hpp>
#include <World.hpp>
#include <PlatformerPhysics2D.hpp>
#include <Jam/LDtkImporter.hpp>
#include "Comps.hpp"
#include "Entity.hpp"
#include <Font.hpp>
#include "Event.hpp"
#include "FrameData.hpp"
#include "Jam/TileMap.hpp"
#include "OpenGLSprite.hpp"
#include "Player.hpp"
#include "Reflection.hpp"
#include "SmallVec.hpp"
#include "Sprite.hpp"
#include <variant>
#include <sstream>
#ifdef TAKO_IMGUI
#include "imgui.h"
#endif


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

inline void ApplyLDtkFields(void* data, nlohmann::json& json, const tako::Reflection::StructInformation* structType)
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

using Rect = tako::Jam::PlatformerPhysics2D::Rect;

inline tako::Vector2 FitMapBound(Rect bounds, tako::Vector2 cameraPos, tako::Vector2 camSize)
{
    cameraPos.x = std::max(bounds.Left() + camSize.x / 2, cameraPos.x);
    cameraPos.x = std::min(bounds.Right() - camSize.x / 2, cameraPos.x);
    cameraPos.y = std::min(bounds.Top() - camSize.y / 2, cameraPos.y);
    cameraPos.y = std::max(bounds.Bottom() + camSize.y / 2, cameraPos.y);

    return cameraPos;
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
		RegisterTileEntity<PlayerSpawn>();
		RegisterTileEntity<Upgrade>([&](tako::World& world, auto ent, auto& entDef)
		{
			world.AddComponent<SpriteRenderer>(ent);
			auto& up = world.GetComponent<Upgrade>(ent);
			auto& ren = world.GetComponent<SpriteRenderer>(ent);
			ren.sprite = m_upgradeSprites[up.upgradeID];
		});
		RegisterTileEntity<Collectible>([&](tako::World& world, auto ent, auto& entDef)
		{
			world.AddComponent<SpriteRenderer>(ent);
			auto& ren = world.GetComponent<SpriteRenderer>(ent);
			ren.sprite = m_collectibleSprite;
		});
	}

	void LoadLevel(int id, std::variant<int, tako::Vector2> coords)
	{
		Player player;
		RigidBody body{{0, 0}, {0, 0, 12, 16}};
		Animator animator{&m_playerAnimation, PlayerIdleClip};
		m_world.IterateComps<Player, RigidBody>([&](Player& pl, RigidBody& rb)
		{
			player = pl;
			body = rb;
		});

		if (std::holds_alternative<int>(coords))
		{
			player.spawnMap = id;
			player.spawnID = std::get<int>(coords);
		}

		m_world.Reset();
		auto& level = m_tileWorld.levels[id];
		m_activeLevel = &level;
		m_activeLevelID = id;
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
		if (std::holds_alternative<tako::Vector2>(coords))
		{
			spawnPos = std::get<tako::Vector2>(coords);
		}
		else
		{
			m_world.IterateComps<Position, PlayerSpawn>([&](Position& pos, PlayerSpawn& spawn)
			{
				if (spawn.id == player.spawnID)
				{
					spawnPos = pos.position;
				}
			});
		}

		m_world.Create
		(
			std::move(player),
			Position{spawnPos},
			std::move(body),
			SpriteRenderer{m_playerAnimation.sprites[0], {0, 1}},
			std::move(animator),
			Camera()
		);
	}

	int GetMaxClockTime()
	{
		ClockMode clockMode;
		m_world.IterateComps<Player>([&](Player& player)
		{
			clockMode = player.clockMode;
		});
		switch (clockMode)
		{
			case ClockMode::Binary: return 2;
			case ClockMode::Decimal: return 10;
			case ClockMode::Hexa: return 16;
		}
	}

	void ResetWorldClock()
	{
		m_worldClock = GetMaxClockTime();
	}

	void UpdateClockText()
	{
		int num = std::ceil(m_worldClock);

		char firstDigit;

		char secondDigit;

		ClockMode clockMode;
		m_world.IterateComps<Player>([&](Player& player)
		{
			clockMode = player.clockMode;
		});
		switch (clockMode)
		{
			case ClockMode::Binary:
			{
				firstDigit = std::min(1, num / 2) + '0';
				secondDigit = num % 2 + '0';
			} break;
			case ClockMode::Decimal:
			{
				if (num >= 10)
				{
					firstDigit = num / 10 + '0';
				}
				else
				{
					firstDigit = '0';
				}
				secondDigit = num % 10 + '0';
			} break;
			case ClockMode::Hexa:
			{
				if (num >= 16)
				{
					firstDigit = num / 16 + '0';
				}
				else
				{
					firstDigit = '0';
				}
				auto mod = num % 16;
				if (mod >= 10)
				{
					secondDigit = mod - 10 + 'A';
				}
				else
				{
					secondDigit = mod + '0';
				}
			} break;
		}
		std::stringstream stream;
		stream << firstDigit << secondDigit;
		auto str = stream.str();
		if (str != m_renderedClockText)
		{
			UpdateText(drawer, m_font, str, m_clockTex);
			m_renderedClockText = str;
		}
	}

	void UpdateBoxText(FrameData* frameData, float dt)
	{
		frameData->showDialog = false;
		frameData->tutorialDialogOpen = false;
		if (sharedData.targetText.size() <= 0) return;
		sharedData.textPassed += dt;
		bool fadeOut = sharedData.textDisplayed == sharedData.targetText.size();
		if (fadeOut && sharedData.textPassed >= (sharedData.textTutorial ? 3 : 1))
		{
			sharedData.targetText = "";
			return;
		}
		frameData->showDialog = sharedData.textDisplayed > 0;
		frameData->tutorialDialogOpen = sharedData.textTutorial && frameData->showDialog;
		if (fadeOut || sharedData.textPassed < 0.1f) return;
		sharedData.textDisplayed++;
		sharedData.textPassed = 0;
		auto str = sharedData.targetText.substr(0, sharedData.textDisplayed);
		UpdateText(drawer, m_font, str, m_dialogTex);
	}

	void InitAudio()
	{
		sharedData.audio->Init();
		m_music = sharedData.audio->Load("/Music.wav");
		sharedData.audio->Play(m_music, true);
		m_gameState = GameState::Title;
		UpdateText(drawer, m_font, "Base Clock", m_titleTex);
	}

	void Setup(const tako::SetupData& setup)
	{
		drawer = new tako::OpenGLPixelArtDrawer(setup.context);
		context = setup.context;
		drawer->SetTargetSize(240, 135);
		drawer->AutoScale();
		sharedData.audio = setup.audio;

		m_font = new tako::Font("/charmap-cellphone.png", 5, 7, 1, 1, 2, 2,
			" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]\a_`abcdefghijklmnopqrstuvwxyz{|}~");
		m_clockTex = CreateText(drawer, m_font, m_renderedClockText);
		m_dialogTex = CreateText(drawer, m_font, " ");
		m_titleTex = CreateText(drawer, m_font, "Press any button");
		m_promptTex = CreateText(drawer, m_font, "   Press [UP] to  \nactivate the clock");
		m_upgradeSprites[0] = drawer->CreateTexture(tako::Bitmap::FromFile("/DashUpgrade.png"));
		m_upgradeSprites[1] = drawer->CreateTexture(tako::Bitmap::FromFile("/HexClock.png"));
		m_upgradeSprites[2] = drawer->CreateTexture(tako::Bitmap::FromFile("/HexClock.png"));


		m_collectibleSprite = drawer->CreateTexture(tako::Bitmap::FromFile("/Collectible.png"));
		auto playerTex = drawer->CreateTexture(tako::Bitmap::FromFile("/Player.png"));
		m_playerAnimation.InitSprites(drawer, playerTex, 12, 18);


		m_tileWorld = tako::Jam::LDtkImporter::LoadWorld("/World.ldtk");
		LoadLevel(0, 0);
		ResetWorldClock();
	}

	void GraphicsUpdate(float dt)
	{
		m_world.IterateComps<SpriteRenderer, Animator>([&](SpriteRenderer& spr, Animator& animator)
		{
			animator.passed += dt;
			auto frameCount = animator.clip.end - animator.clip.start + 1;
			auto totalDuration = animator.clip.duration * frameCount;
			while (animator.passed >= totalDuration)
			{
				animator.passed -= totalDuration;
			}
			size_t frame = animator.clip.start + std::floor(animator.passed / animator.clip.duration);
			spr.sprite = animator.flipX ? animator.data->reverse[frame] : animator.data->sprites[frame];
		});

		m_world.IterateComps<Position, Camera>([&](Position& pos, Camera& cam)
		{
			auto camSize = drawer->GetCameraViewSize();
			Rect bounds(m_activeLevel->size.x/2, m_activeLevel->size.y/2, m_activeLevel->size.x, m_activeLevel->size.y);
			auto target = FitMapBound(bounds, pos.position, camSize);
			if (cam.snapped)
			{
				cam.position += (target - cam.position) * dt * 6;
				cam.position = FitMapBound(bounds, cam.position, camSize);
			}
			else
			{
				cam.position = target;
				cam.snapped = true;
			}
			drawer->SetCameraPosition(cam.position);
		});
	}


	void Update(const tako::GameStageData stageData, tako::Input* input, float dt)
	{
		if (m_gameState == GameState::AudioInit)
		{
			if (input->GetAnyDown())
			{
				InitAudio();
				m_world.IterateComps<Player>([&](Player& player)
				{
					player.grounded = true;
				});
			}
			else
			{
				return;
			}
		}
		else if (m_gameState == GameState::Title)
		{
			static float prevAxisY = 0;
			auto axisY = input->GetAxis(tako::Axis::Left).y;
			auto axisUpDown = axisY > 0.9f && prevAxisY < 0.9f;
			prevAxisY = axisY;

			if (axisUpDown || input->GetKeyDown(tako::Key::Up) || input->GetKeyDown(tako::Key::W) || input->GetKeyDown(tako::Key::X) || input->GetKeyDown(tako::Key::Gamepad_Dpad_Up) || input->GetKeyDown(tako::Key::Gamepad_B))
			{
				m_gameState = GameState::Game;
			}
			else
			{
				GraphicsUpdate(dt);
				return;
			}
		}
		auto frameData = reinterpret_cast<FrameData*>(stageData.frameData);
#ifdef TAKO_IMGUI
		m_world.IterateComps<Position, Player>([&](Position& pPos, Player& player)
		{
			ImGui::Begin("Debug");
			ImGui::InputInt("Spawn Map", &player.spawnMap);
			ImGui::InputInt("Spawner ID", &player.spawnID);
			ImGui::InputInt("ClockMode", reinterpret_cast<int*>(&player.clockMode));

			ImGui::Checkbox("Dash", &player.unlocked[0]);
			ImGui::Text("Collected: %d", frameData->collectedCount);
			ImGui::End();
		});

#endif
		UpdateBoxText(frameData, dt);
		if (frameData->tutorialDialogOpen)
		{
			return;
		}

		tako::SmallVec<tako::Entity, 4> toDelete;
		std::optional<int> newNeighbourID;
#ifndef NDEBUG
		if (input->GetKeyDown(tako::Key::Enter))
		{
			m_world.IterateComps<Player, Position>([&](Player& player, Position& pos)
			{
				m_tileWorld = tako::Jam::LDtkImporter::LoadWorld("/World.ldtk");
				m_playerWarp = player;
			});
			ResetWorldClock();
		}
#endif // !NDEBUG
		if (m_playerWarp)
		{
			auto player = m_playerWarp.value();
			LoadLevel(player.spawnMap, player.spawnID);
			m_playerWarp = {};
		}
		else
		{
			std::optional<tako::Vector2> newPos;
			m_world.IterateComps<Player, Position>([&](Player& player, Position& pos)
			{
				if (pos.position.x < 0 || pos.position.x > m_activeLevel->size.x || pos.position.y < 0 || pos.position.y > m_activeLevel->size.y)
				{
					tako::Vector2 worldPos(pos.position.x + m_activeLevel->worldX, m_activeLevel->worldY + m_activeLevel->size.y - pos.position.y);
					for (auto neighbourID : m_activeLevel->neighbours)
					{
						auto& neighbour = m_tileWorld.levels[neighbourID];
						if (worldPos.x >= neighbour.worldX && worldPos.x <= neighbour.worldX + neighbour.size.x && worldPos.y >= neighbour.worldY && worldPos.y <= neighbour.worldY + neighbour.size.y )
						{
							newPos = tako::Vector2(worldPos.x - neighbour.worldX, neighbour.worldY + neighbour.size.y - worldPos.y);
							newNeighbourID = neighbourID;
							break;
						}
					}
				}
			});
			if (newPos)
			{
				LoadLevel(newNeighbourID.value(), newPos.value());
			}
		}
		PlayerUpdate(&sharedData, frameData, input, dt, m_world, m_activeLevelID);

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
		m_world.IterateComps<Player, RigidBody>([&](Player& player, RigidBody& rb)
		{
			player.grounded = rb.velocity.y == 0 && (player.grounded || player.prevYVelocity < 0);
			player.prevYVelocity = rb.velocity.y;
		});
		m_worldClock -= dt;
		if (frameData->triggeredCheckpoint)
		{
			ResetWorldClock();
		}
		m_worldClock = std::min(m_worldClock, (float) GetMaxClockTime());
		if (m_worldClock <= 0)
		{
			ResetWorldClock();
			sharedData.audio->Play("/Reset.wav");
			m_world.IterateComps<Position, Player>([&](Position& pPos, Player& player)
			{
				if (m_activeLevelID != player.spawnMap)
				{
					m_playerWarp = player;
				}
				else
				{
					m_world.IterateComps<Position, PlayerSpawn>([&](Position& pos, PlayerSpawn& spawn)
					{
						if (player.spawnID == spawn.id)
						{
							pPos.position = pos.position;
							player.grounded = true;
						}
					});
				}
			});

		}

		m_world.IterateComps<tako::Entity, SpriteRenderer, FadeOut>([&](tako::Entity ent, SpriteRenderer& spr, FadeOut& fade)
		{
			fade.left -= dt;
			if (fade.left <= 0)
			{
				toDelete.Push(ent);
				return;
			}
			spr.alpha = fade.left / fade.duration * fade.startFade;
		});

		UpdateClockText();
		for (int i = 0; i < toDelete.GetLength(); i++)
		{
			m_world.Delete(toDelete[i]);
		}
		GraphicsUpdate(dt);
	}

	void DrawEntities()
	{
		m_world.IterateComps<Position, RectRenderer>([&](Position& pos, RectRenderer& ren)
		{
			drawer->DrawRectangle(pos.position.x - ren.size.x / 2, pos.position.y + ren.size.y / 2, ren.size.x, ren.size.y, ren.color);
		});
		m_world.IterateComps<Position, SpriteRenderer>([&](Position& pos, SpriteRenderer& ren)
		{
			float width;
			float height;
			if (std::holds_alternative<tako::Texture>(ren.sprite))
			{
				auto tex = std::get<tako::Texture>(ren.sprite);
				width = tex.width;
				height = tex.height;
			}
			else
			{
				auto tex = std::get<tako::OpenGLSprite*>(ren.sprite);
				width = tex->width;
				height = tex->height;
			}
			float x = pos.position.x + ren.offset.x - width / 2;
			float y = pos.position.y + ren.offset.y + height / 2;
			if (std::holds_alternative<tako::Texture>(ren.sprite))
			{
				auto tex = std::get<tako::Texture>(ren.sprite);
				drawer->DrawImage(x, y, width, height, tex.handle);
			}
			else
			{
				auto sprite = std::get<tako::OpenGLSprite*>(ren.sprite);
				drawer->DrawSprite(x, y, width, height, sprite, {255, 255, 255, ren.alpha});
			}
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
		if (m_gameState == GameState::AudioInit)
		{
			drawer->SetClearColor({0, 0, 0, 255});
			drawer->Clear();
			drawer->SetCameraPosition({0 , 0});
			drawer->DrawImage((float) -m_titleTex.width / 2, (float) m_titleTex.height / 2, m_titleTex.width, m_titleTex.height, m_titleTex.handle);
			return;
		}
		auto frameData = reinterpret_cast<FrameData*>(stageData.frameData);
		m_world.IterateComps<Camera>([&](Camera& cam)
		{
			drawer->SetCameraPosition(cam.position);
		});
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

		drawer->SetCameraPosition({0 , 0});
		if (m_gameState == GameState::Title)
		{
			drawer->DrawImage((float) -m_titleTex.width, 42, m_titleTex.width * 2, m_titleTex.height * 2, m_titleTex.handle);
			drawer->DrawImage((float) -m_promptTex.width / 2, (float) m_promptTex.height / 2, m_promptTex.width, m_promptTex.height, m_promptTex.handle);
		}
		else
		{
			drawer->DrawImage((float) -m_clockTex.width / 2, 42, m_clockTex.width, m_clockTex.height, m_clockTex.handle);
			if (frameData->showDialog)
			{
				float x = (float) -m_dialogTex.width / 2;
				float y = 0;
				float width = m_dialogTex.width;
				float height = m_dialogTex.height;
				constexpr const float padding = 4;
				drawer->DrawRectangle(x - padding / 2, y + padding / 2, width + padding, height + padding, {0, 0, 0, 255});
				drawer->DrawImage(x, y, width, height, m_dialogTex.handle);
			}
		}

	}

private:
	tako::OpenGLPixelArtDrawer* drawer;
	tako::GraphicsContext* context;
	tako::World m_world;
	tako::Jam::TileWorld m_tileWorld;
	tako::Jam::TileMap* m_activeLevel;
	int m_activeLevelID;
	float m_worldClock;
	std::vector<tako::Texture> m_layerCache;
	std::vector<tako::Jam::PlatformerPhysics2D::Node> m_nodesCache;
	std::map<std::string, std::function<tako::Entity(tako::World&, tako::Jam::TileEntity&)>> m_entityInstantiate;

	tako::Font* m_font;
	std::string m_renderedClockText = "10";
	tako::Texture m_clockTex;
	tako::Texture m_titleTex;
	tako::Texture m_promptTex;
	tako::Texture m_dialogTex;
	tako::Texture m_collectibleSprite;
	AnimationData m_playerAnimation;
	std::array<tako::Texture, 3> m_upgradeSprites;
	std::optional<Player> m_playerWarp;
	tako::AudioClip* m_music;
	SharedData sharedData;
	GameState m_gameState = GameState::AudioInit;
};
