#include "engine.h"
#include "mainloop.h"

using namespace std;

int main(int argc, const char *const *argv)
{
	MainLoop mainloop = MainLoop();
	auto engine = make_shared<Engine>();

	mainloop
		.setFixedResolution(false)
		.setUsagiMode()
		.setEngine(engine)
		.setTitle("P.E.N.G.U.I.N!")
		.setAppName("PENGUIN");

	mainloop.setPreferredDisplayResolution(800, 600);

	mainloop.init(argc, argv);
	engine->init();
	mainloop.run();
}
