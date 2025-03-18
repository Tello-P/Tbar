#include <stdio.h>        	// Standard input/output (e.g., printf)
#include <stdlib.h>       	// Standard library (e.g., exit)
#include <SDL2/SDL.h>     	// SDL2 library for creating windows and rendering
#include <SDL2/SDL_syswm.h> 	// SDL2 subsystem info for accessing native window system (X11)
#include <X11/Xlib.h>     	// X11 library for low-level window management
#include <X11/Xatom.h>    	// X11 atoms for window properties (e.g., _NET_WM_WINDOW_TYPE)


// CONFIGS
#define BAR_WIDTH 1920    	// Width of the bar, adjust to your screen resolution (e.g., 1920 for 1920x1080)
#define BAR_HEIGHT 30     	// Height of the bar, similar to Polybar or other status bars
#define BG_RED_INTENSITY 0	// (0-255)
#define BG_GREEN_INTENSITY 200	// (0-255)
#define BG_BLUE_INTENSITY 0	// (0-255)
#define BG_COLOR_INTENSITY 255	// (0-255)





// Function to set the window as a dock-type window with X11 properties
void set_dock_properties(SDL_Window *window) 
{
	// Get system window manager info from SDL
	SDL_SysWMinfo wm_info;
	SDL_VERSION(&wm_info.version); // Set the version of SDL_SysWMinfo to the current SDL version

	if (!SDL_GetWindowWMInfo(window, &wm_info)) {
		// If this fails, SDL couldn't retrieve native window info
		printf("Error getting WM info: %s\n", SDL_GetError());
		return;
	}

	// Check if we're using X11 (Linux window system)
	if (wm_info.subsystem != SDL_SYSWM_X11) {
		printf("Not using X11, dock properties cannot be applied.\n");
		// Example: This would fail on Wayland or Windows, but we're on Debian with i3 (X11)
		return;
	}

	
	// Access X11-specific data
	Display *display = wm_info.info.x11.display; // The X11 display connection
	Window xwindow = wm_info.info.x11.window;    // The native X11 window handle

	
	// Set the window type to DOCK (like a status bar or dock)
	Atom window_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False); // Atom for window type property
	Atom dock_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False); // Atom for dock type
	XChangeProperty(display, xwindow, window_type, XA_ATOM, 32, PropModeReplace, (unsigned char *)&dock_type, 1);
	// Example: This tells i3 "treat this window like Polybar or a dock", making it special

	
	// Make the window "sticky" so it appears on all workspaces
	Atom state = XInternAtom(display, "_NET_WM_STATE", False); // Atom for window state property
	Atom sticky = XInternAtom(display, "_NET_WM_STATE_STICKY", False); // Atom to make it sticky
	XChangeProperty(display, xwindow, state, XA_ATOM, 32, PropModeReplace, (unsigned char *)&sticky, 1);
	// Example: Without this, the bar would disappear when switching from workspace 1 to 2

	
	// Reserve screen space for the bar (prevents other windows from overlapping)
	Atom strut_partial = XInternAtom(display, "_NET_WM_STRUT_PARTIAL", False); // Atom for reserving space
	long struts[12] = {0}; 				// Array to define reserved areas (left, right, top, bottom, etc.)
	struts[2] = BAR_HEIGHT; 			// Reserve BAR_HEIGHT pixels at the top of the screen
	struts[8] = 0;          			// Start of reserved area (left edge, 0 pixels from left)
	struts[9] = BAR_WIDTH;  			// End of reserved area (right edge, full width)
	XChangeProperty(display, xwindow, strut_partial, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)struts, 12);
	// Example: If BAR_HEIGHT is 30, no maximized window will cover the top 30 pixels


	// Ensure changes are applied immediately
	XFlush(display);
	printf("Dock properties applied successfully.\n");
}



int main()
{
	// Initialize SDL with video subsystem
	if (SDL_Init(SDL_INIT_VIDEO) != 0) 
	{
		printf("Error initializing SDL: %s\n", SDL_GetError());
		return 1; // Exit if SDL fails to start
	}

	// Set a hint to tell SDL this is a dock window (optional, reinforces X11 properties)
	SDL_SetHint(SDL_HINT_X11_WINDOW_TYPE, "_NET_WM_WINDOW_TYPE_DOCK");
	// Example: This is like a "suggestion" to SDL before creating the window

	// Create the window for the bar
	SDL_Window* window = SDL_CreateWindow(
			"Tbar",              // Window title (shows up in i3 and xprop)
			0, 0,                // Position at top-left corner (x=0, y=0)
			BAR_WIDTH, BAR_HEIGHT, // Size of the window
			SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS // Show immediately, no borders
			);


	if (!window)
	{
		printf("Error creating window: %s\n", SDL_GetError());
		SDL_Quit();
		return 1; // Exit if window creation fails
	}

	// Apply dock properties right after window creation
	set_dock_properties(window);
	// Example: This ensures the window is set as a dock before rendering starts

	// Create a renderer for drawing on the window
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	if (!renderer)
	{
		printf("Error creating renderer: %s\n", SDL_GetError());
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1; // Exit if renderer fails
	}

	// Set background color to light gray (RGB: 200, 200, 200, fully opaque)
	SDL_SetRenderDrawColor(renderer, BG_RED_INTENSITY, BG_GREEN_INTENSITY, BG_BLUE_INTENSITY, BG_COLOR_INTENSITY);
	SDL_RenderClear(renderer); // Fill the window with the background color
	SDL_RenderPresent(renderer); // Display the rendered content

	// Event loop to keep the window open and responsive
	SDL_Event event;
	int running = 1;
	while (running)
	{
		while (SDL_PollEvent(&event)) 
		{
			if (event.type == SDL_QUIT)
			{
				running = 0; // Exit when the window is closed (e.g., Alt+F4 or i3 kill)
			}
			// Example: You could add more events here, like mouse clicks to show a menu
		}
		// Example: Add dynamic content here, e.g., SDL_RenderDrawLine for a clock
		SDL_Delay(1000);
	}

	// Clean up resources before exiting
	SDL_DestroyRenderer(renderer); // Free the renderer
	SDL_DestroyWindow(window);     // Destroy the window
	SDL_Quit();                    // Shut down SDL
	return 0;                      // Successful exit

}
