#include "Butterfly.hpp"
#include "EditorApplication.hpp"

int main()
{
	Butterfly::Application* app = new Butterfly::Application();
	app->SetApplicationExtention(new Butterfly::EditorApplication());
	app->Init();
	delete app;
}