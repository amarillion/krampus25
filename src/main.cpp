#include "engine.h"
#include "mainloop.h"

using namespace std;

int main(int argc, const char *const *argv)
{
	MainLoop mainloop = MainLoop();

	mainloop
		.setFixedResolution(false)
		.setUsagiMode()
		.setTitle("P.E.N.G.U.I.N!")
		.setAppName("PENGUIN");
	
	mainloop.setPreferredDisplayResolution(1024, 768);
	mainloop.init(argc, argv);

	auto engine = make_shared<Engine>();
	engine->init();
	mainloop.run(engine.get());
}
