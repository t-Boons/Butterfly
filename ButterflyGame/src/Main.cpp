#include "Butterfly.hpp"
#include "Core/GameApplication.hpp"

int main()
{
	Butterfly::Application* app = new Butterfly::Application();
	app->SetApplicationExtention(new Butterfly::Game::GameApplication());
	app->Init();
	delete app;
}