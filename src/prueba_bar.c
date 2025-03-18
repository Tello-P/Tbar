#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define BAR_WIDTH 1920
#define BAR_HEIGHT 30
#define BG_COLOR 0x212026      
#define TIME_COLOR "white" // Color para la hora
#define DATE_COLOR "white"  // Color para la fecha
#define TIME_POS 1920/2    // Posición x para la hora (centro de la barra)
#define DATE_POS TIME_POS - 110 // Posición x para la fecha (a la izquierda de la hora)
#define TEXT_SIZE 10.00    // Tamaño del texto

// Configurar propiedades de dock
void set_dock_properties(Display *display, Window window) {
	Atom window_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
	Atom dock_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);
	XChangeProperty(display, window, window_type, XA_ATOM, 32, PropModeReplace, (unsigned char *)&dock_type, 1);

	Atom state = XInternAtom(display, "_NET_WM_STATE", False);
	Atom sticky = XInternAtom(display, "_NET_WM_STATE_STICKY", False);
	XChangeProperty(display, window, state, XA_ATOM, 32, PropModeReplace, (unsigned char *)&sticky, 1);

	Atom strut_partial = XInternAtom(display, "_NET_WM_STRUT_PARTIAL", False);
	long struts[12] = {0};
	struts[2] = BAR_HEIGHT;
	struts[8] = 0;
	struts[9] = BAR_WIDTH;
	XChangeProperty(display, window, strut_partial, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)struts, 12);

	XMoveWindow(display, window, 0, 0);
}

// Actualizar el contenido de la barra (hora y fecha)
void update_bar(Display *display, Window window, XftDraw *draw, XftColor *time_color, XftColor *date_color, XftFont *font) {
	XSetWindowBackground(display, window, BG_COLOR);
	XClearWindow(display, window);

	time_t now = time(NULL);
	struct tm *tm_info = localtime(&now);

	// Hora
	char time_str[32];
	strftime(time_str, sizeof(time_str), "%H:%M", tm_info);
	XftDrawStringUtf8(draw, time_color, font, TIME_POS, 20, (FcChar8 *)time_str, strlen(time_str));

	// Fecha
	char date_str[32];
	strftime(date_str, sizeof(date_str), "%d-%m-%Y", tm_info);
	XftDrawStringUtf8(draw, date_color, font, DATE_POS, 20, (FcChar8 *)date_str, strlen(date_str));
}

int main() {
	// Abrir conexión con el servidor X
	Display *display = XOpenDisplay(NULL);
	if (!display) {
		printf("Error: Cannot open display\n");
		return 1;
	}

	// Obtener pantalla y ventana raíz
	int screen = DefaultScreen(display);
	Window root = RootWindow(display, screen);

	// Crear la ventana
	Window bar = XCreateSimpleWindow(display, root, 0, 0, BAR_WIDTH, BAR_HEIGHT, 0,
			BlackPixel(display, screen), BG_COLOR);

	// Configurar propiedades de dock
	set_dock_properties(display, bar);

	// Hacer la ventana visible
	XMapWindow(display, bar);

	// Configurar Xft para dibujar texto
	XftFont *font = XftFontOpen(display, screen,
			XFT_FAMILY, XftTypeString, "sans",
			XFT_SIZE, XftTypeDouble, TEXT_SIZE,
			NULL);
	if (!font) {
		printf("Error: Cannot load font\n");
		XCloseDisplay(display);
		return 1;
	}

	// Configurar colores para hora y fecha
	XftDraw *draw = XftDrawCreate(display, bar, DefaultVisual(display, screen), DefaultColormap(display, screen));
	XftColor time_color;
	XftColor date_color;
	XftColorAllocName(display, DefaultVisual(display, screen), DefaultColormap(display, screen), TIME_COLOR, &time_color);
	XftColorAllocName(display, DefaultVisual(display, screen), DefaultColormap(display, screen), DATE_COLOR, &date_color);

	// Bucle principal para actualizar la barra
	while (1) {
		update_bar(display, bar, draw, &time_color, &date_color, font);
		XFlush(display);
		sleep(1);
	}

	// Liberar recursos
	XftColorFree(display, DefaultVisual(display, screen), DefaultColormap(display, screen), &time_color);
	XftColorFree(display, DefaultVisual(display, screen), DefaultColormap(display, screen), &date_color);
	XftDrawDestroy(draw);
	XftFontClose(display, font);
	XDestroyWindow(display, bar);
	XCloseDisplay(display);
	return 0;
}
