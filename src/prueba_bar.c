#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // Para sleep()

#define BAR_WIDTH 1920 
#define BAR_HEIGHT 30 
#define BG_COLOR 0xC8C8C8


void set_dock_properties(Display *display, Window window) {
    // Set window type to DOCK
    Atom window_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    Atom dock_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);
    XChangeProperty(display, window, window_type, XA_ATOM, 32, PropModeReplace, (unsigned char *)&dock_type, 1);

    // Make it sticky (visible on all workspaces)
    Atom state = XInternAtom(display, "_NET_WM_STATE", False);
    Atom sticky = XInternAtom(display, "_NET_WM_STATE_STICKY", False);
    XChangeProperty(display, window, state, XA_ATOM, 32, PropModeReplace, (unsigned char *)&sticky, 1);

    // Reserve space ONLY at the top using _NET_WM_STRUT_PARTIAL
    Atom strut_partial = XInternAtom(display, "_NET_WM_STRUT_PARTIAL", False);
    long struts[12] = {0}; // Initialize all to zero
    struts[2] = BAR_HEIGHT; // Top strut: reserve BAR_HEIGHT pixels at the top
    struts[8] = 0;          // Left start of top strut
    struts[9] = BAR_WIDTH;  // Right end of top strut
    XChangeProperty(display, window, strut_partial, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)struts, 12);

    // Set background color to light gray (RGB: 200, 200, 200)
    XSetWindowBackground(display, window, 0xC8C8C8); // Hex for 200,200,200
    XClearWindow(display, window); // Apply the background color

    // Ensure the window stays at the top-left corner
    XMoveWindow(display, window, 0, 0);
}

int main() {
    // Open connection to the X server
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        printf("Error: Cannot open display\n");
        return 1;
    }

    // Get the default screen and root window
    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    // Create a simple window
    Window bar = XCreateSimpleWindow(display, root, 
                                     0, 0,           // Position: top-left corner
                                     BAR_WIDTH, BAR_HEIGHT, // Size
                                     0,              // Border width
                                     BlackPixel(display, screen), // Border color (not used)
                                     BG_COLOR);      // Background color (light gray)

    // Set window properties to make it a dock
    set_dock_properties(display, bar);

    // Map the window (make it visible)
    XMapWindow(display, bar);

    // Flush changes to the X server
    XFlush(display);

    // Simple event loop to keep the window alive
    XEvent event;
    while (1) {
        XNextEvent(display, &event); // Wait for events
        if (event.type == ClientMessage) { // Handle close event
            break;
        }
    }

    // Clean up and close the display
    XDestroyWindow(display, bar);
    XCloseDisplay(display);
    return 0;
}
