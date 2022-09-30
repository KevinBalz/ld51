#pragma once
#include <Tako.hpp>
#include <OpenGLPixelArtDrawer.hpp>

class Game
{
public:
	Game()
	{}

	void Setup(const tako::SetupData& setup)
	{
		drawer = new tako::OpenGLPixelArtDrawer(setup.context);
		context = setup.context;
	}


	void Update(const tako::GameStageData stageData, tako::Input* input, float dt)
	{
	}

	void Draw(const tako::GameStageData stageData)
	{
		drawer->Resize(context->GetWidth(), context->GetHeight());
		drawer->Clear();
	}

private:
	tako::OpenGLPixelArtDrawer* drawer;
	tako::GraphicsContext* context;
};
