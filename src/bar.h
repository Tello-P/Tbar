#ifndef BAR_H
#define BAR_H

#include <xcb/xcb.h>

typedef struct {
	xcb_connection_t *conn;
	xcb_screen_t *screen;
	xcb_window_t win;
	xcb_gcontext_t gc;
	int width;
	int height;
} Bar;

Bar* bar_init(int width, int height);
void bar_draw(Bar *bar, const char *text, int x, int y);
void bar_free(Bar *bar);

#endif
