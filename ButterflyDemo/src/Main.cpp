#include "Butterfly.hpp"
#include "App.hpp"

int main()
{
	Butterfly::Application* app = new Butterfly::Application();
	app->AttachLayer<Butterfly::SandboxLayer>();
	app->Start();
	delete app;
}