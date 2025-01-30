#include "App.hpp"

int main()
{
	Butterfly::App* app = new Butterfly::App();
	app->Init();

	while (!app->ShouldShutDown)
	{
		app->Tick();
	}
	app->ShutDown();
	delete app;
}