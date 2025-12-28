#include "engine.h"
#include "simpleloop.h"

using namespace std;

int main(int argc, const char *const *argv)
{
	Simple::MainLoop mainloop = Simple::MainLoop();
	auto engine = make_shared<Engine>();

	mainloop
		.setFixedResolution(false)
		.setUsagiMode()
		.setTitle("P.E.N.G.U.I.N!")
		.setAppName("PENGUIN")
		.setApp(engine);

	mainloop.setPreferredDisplayResolution(1024, 768);

	mainloop.init(argc, argv);
	engine->init();
	mainloop.run();
}
