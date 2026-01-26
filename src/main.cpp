#include "engine.h"
#include "mainloop.h"
#include "autoscalecanvas.h"

using namespace std;

int main(int argc, const char *const *argv)
{
	MainLoop mainloop = MainLoop();

	mainloop
		.setTitle("P.E.N.G.U.I.N!")
		.setAppName("PENGUIN")
		.setResizableWindow(true)
		.setPreferredDisplaySize(1024, 768)
		.setMouseEnabled(true);

	if (!mainloop.init(argc, argv)) {
		auto engine = make_shared<Engine>();
		engine->init();
		mainloop.run(engine.get());
	}
}
