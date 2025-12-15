/* An example demonstrating different blending modes.
 */
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_ttf.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include "demo.h"
#include "abort.h"

using namespace std;

struct Example {
	ALLEGRO_EVENT_QUEUE *queue;

	// double timer[4], counter[4];
	int FPS;

	unique_ptr<Demo> demo;
} ex;

static void tick(void)
{
	ex.demo->draw();
	al_flip_display();
}

static void run(void)
{
	ALLEGRO_EVENT event;
	bool need_draw = true;

	while (1) {
		if (need_draw && al_is_event_queue_empty(ex.queue)) {
			tick();
			need_draw = false;
		}

		al_wait_for_event(ex.queue, &event);

		switch (event.type) {
			case ALLEGRO_EVENT_DISPLAY_CLOSE:
				return;

			case ALLEGRO_EVENT_KEY_DOWN:
				if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
					return;
				break;

			case ALLEGRO_EVENT_TIMER:
				need_draw = true;
				break;
		}
	}
}

static void init(void)
{
	printf("Initializing example...\n");
	ex.FPS = 60;

	ex.demo = Demo::newInstance();
	ex.demo->init();
}

int main(int argc, char **argv)
{
	ALLEGRO_DISPLAY *display;
	ALLEGRO_TIMER *timer;

	(void)argc;
	(void)argv;

	if (!al_init()) {
		abort_example("Could not init Allegro.\n");
	}

	al_install_keyboard();
	al_install_mouse();
	al_init_image_addon();
	al_init_font_addon();
	al_init_ttf_addon();

	display = al_create_display(640, 480);
	if (!display) {
		abort_example("Error creating display\n");
	}

	init();

	timer = al_create_timer(1.0 / ex.FPS);

	ex.queue = al_create_event_queue();
	al_register_event_source(ex.queue, al_get_keyboard_event_source());
	al_register_event_source(ex.queue, al_get_mouse_event_source());
	al_register_event_source(ex.queue, al_get_display_event_source(display));
	al_register_event_source(ex.queue, al_get_timer_event_source(timer));

	al_start_timer(timer);
	run();

	al_destroy_event_queue(ex.queue);  

	return 0;
}
