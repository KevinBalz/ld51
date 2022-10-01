#include "FrameData.hpp"
#include "Tako.hpp"
#include "Game.hpp"

void Setup(void* gameDataPtr, const tako::SetupData& setup)
{
	auto* gameData = reinterpret_cast<Game*>(gameDataPtr);
	new (gameData) Game();
	gameData->Setup(setup);
}

void Update(const tako::GameStageData stageData, tako::Input* input, float dt)
{
	auto* gameData = reinterpret_cast<Game*>(stageData.gameData);
	auto* frameData = reinterpret_cast<FrameData*>(stageData.frameData);
	new (frameData) FrameData();
	gameData->Update(stageData, input, dt);
}

void Draw(const tako::GameStageData stageData)
{
	auto* gameData = reinterpret_cast<Game*>(stageData.gameData);
	gameData->Draw(stageData);
}

void tako::InitTakoConfig(GameConfig& config)
{
	config.Setup = Setup;
	config.Update = Update;
	config.Draw = Draw;
	config.graphicsAPI = tako::GraphicsAPI::OpenGL;
	config.initAudioDelayed = true;
	config.gameDataSize = sizeof(Game);
	config.frameDataSize = sizeof(FrameData);
}
