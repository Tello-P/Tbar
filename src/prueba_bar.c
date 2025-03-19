#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/statvfs.h> // Para espacio en disco
#include <sys/sysinfo.h> // Para RAM y CPU

#define BAR_WIDTH 1920
#define BAR_HEIGHT 30
#define BG_COLOR 0x212026       // Fondo gris oscuro
#define TEXT_COLOR "light gray" // Color para todos los textos
#define SEP_COLOR "cyan"        // Color para separadores
#define TIME_POS 1920 - 60      // Hora a la derecha
#define DATE_POS TIME_POS - 110 // Fecha a la izquierda de la hora
#define BATTERY_POS TIME_POS - 220 // Batería a la izquierda de la fecha
#define VOL_POS BATTERY_POS - 100  // Volumen a la izquierda de la batería
#define DISK_POS VOL_POS - 100     // Espacio en disco a la izquierda del volumen
#define RAM_POS DISK_POS - 100     // RAM a la izquierda del disco
#define CPU_POS RAM_POS - 100      // CPU a la izquierda de la RAM
#define WORKSPACE_POS 10           // Workspace a la izquierda
#define WINDOW_POS 65              // Nombre de la ventana después del workspace
#define SEP "| "                   // Separador entre módulos
#define TEXT_SIZE 10.00            // Tamaño del texto

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

// Obtener porcentaje de batería
int get_battery_percentage() {
    FILE *fp = fopen("/sys/class/power_supply/BAT0/capacity", "r");
    if (!fp) return -1;
    int percentage;
    fscanf(fp, "%d", &percentage);
    fclose(fp);
    return percentage;
}

// Obtener el workspace actual
int get_current_workspace(Display *display, Window root) {
    Atom current_desktop = XInternAtom(display, "_NET_CURRENT_DESKTOP", False);
    Atom type;
    int format;
    unsigned long nitems, bytes_after;
    unsigned char *data = NULL;
    if (XGetWindowProperty(display, root, current_desktop, 0, 1, False, XA_CARDINAL,
                           &type, &format, &nitems, &bytes_after, &data) == Success && data) {
        int workspace = *(int *)data;
        XFree(data);
        return workspace + 1;
    }
    return -1;
}

// Obtener el nombre de la ventana activa
char *get_active_window_name(Display *display, Window root) {
    Atom active_window = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
    Atom wm_name = XInternAtom(display, "_NET_WM_NAME", False);
    Atom utf8_string = XInternAtom(display, "UTF8_STRING", False);
    Atom type;
    int format;
    unsigned long nitems, bytes_after;
    unsigned char *data = NULL;

    if (XGetWindowProperty(display, root, active_window, 0, 1, False, XA_WINDOW,
                           &type, &format, &nitems, &bytes_after, &data) == Success && data) {
        Window win = *(Window *)data;
        XFree(data);

        if (XGetWindowProperty(display, win, wm_name, 0, 1024, False, utf8_string,
                               &type, &format, &nitems, &bytes_after, &data) == Success && data) {
            char *name = strdup((char *)data);
            XFree(data);
            return name;
        }
    }
    return strdup("N/A");
}

// Obtener uso de RAM
int get_ram_usage() {
    struct sysinfo info;
    if (sysinfo(&info) != 0) return -1;
    unsigned long total = info.totalram;
    unsigned long free = info.freeram;
    return (int)(((total - free) * 100) / total);
}

// Obtener uso de CPU (aproximación simple)
int get_cpu_usage() {
    static long long prev_idle = 0, prev_total = 0;
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return -1;

    long long user, nice, system, idle, iowait, irq, softirq;
    fscanf(fp, "cpu %lld %lld %lld %lld %lld %lld %lld", &user, &nice, &system, &idle, &iowait, &irq, &softirq);
    fclose(fp);

    long long total = user + nice + system + idle + iowait + irq + softirq;
    long long idle_time = idle + iowait;
    long long total_diff = total - prev_total;
    long long idle_diff = idle_time - prev_idle;

    prev_total = total;
    prev_idle = idle_time;

    if (total_diff == 0) return 0;
    return (int)(100 * (total_diff - idle_diff) / total_diff);
}

// Obtener espacio en disco
int get_disk_usage() {
    struct statvfs stat;
    if (statvfs("/", &stat) != 0) return -1;
    unsigned long total = stat.f_blocks * stat.f_frsize;
    unsigned long free = stat.f_bfree * stat.f_frsize;
    return (int)(((total - free) * 100) / total);
}

// Obtener volumen (simulación, ajusta según tu sistema)
int get_volume() {
    // Esto es un placeholder. Necesitarías usar ALSA o PulseAudio para obtener el volumen real.
    // Por ahora, devolvemos un valor fijo.
    return 50; // Simula 50%
}

void update_bar(Display *display, Window window, XftDraw *draw, XftColor *text_color, XftColor *sep_color, XftFont *font) {
    XSetWindowBackground(display, window, BG_COLOR);
    XClearWindow(display, window);

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    // Workspace
    char workspace_str[32];
    int workspace = get_current_workspace(display, RootWindow(display, DefaultScreen(display)));
    if (workspace >= 0) {
        snprintf(workspace_str, sizeof(workspace_str), "%d", workspace);
    } else {
        strcpy(workspace_str, "N/A");
    }
    XftDrawStringUtf8(draw, text_color, font, WORKSPACE_POS, 20, (FcChar8 *)workspace_str, strlen(workspace_str));

    // Separador
    XftDrawStringUtf8(draw, sep_color, font, WORKSPACE_POS + 20, 20, (FcChar8 *)SEP, strlen(SEP));

    // Nombre de la ventana
    char *window_name = get_active_window_name(display, RootWindow(display, DefaultScreen(display)));
    XftDrawStringUtf8(draw, text_color, font, WINDOW_POS, 20, (FcChar8 *)window_name, strlen(window_name));
    free(window_name);

    // CPU
    char cpu_str[32];
    int cpu = get_cpu_usage();
    snprintf(cpu_str, sizeof(cpu_str), "CPU %d%%", cpu);
    XftDrawStringUtf8(draw, text_color, font, CPU_POS, 20, (FcChar8 *)cpu_str, strlen(cpu_str));

    // Separador
    XftDrawStringUtf8(draw, sep_color, font, CPU_POS + 60, 20, (FcChar8 *)SEP, strlen(SEP));

    // RAM
    char ram_str[32];
    int ram = get_ram_usage();
    snprintf(ram_str, sizeof(ram_str), "RAM %d%%", ram);
    XftDrawStringUtf8(draw, text_color, font, RAM_POS, 20, (FcChar8 *)ram_str, strlen(ram_str));

    // Separador
    XftDrawStringUtf8(draw, sep_color, font, RAM_POS + 60, 20, (FcChar8 *)SEP, strlen(SEP));

    // Espacio en disco
    char disk_str[32];
    int disk = get_disk_usage();
    snprintf(disk_str, sizeof(disk_str), "eS: %d%%", disk);
    XftDrawStringUtf8(draw, text_color, font, DISK_POS, 20, (FcChar8 *)disk_str, strlen(disk_str));

    // Separador
    XftDrawStringUtf8(draw, sep_color, font, DISK_POS + 60, 20, (FcChar8 *)SEP, strlen(SEP));

    // Volumen (simulado)
    char vol_str[32];
    int vol = get_volume();
    snprintf(vol_str, sizeof(vol_str), "VOL %d%%", vol);
    XftDrawStringUtf8(draw, text_color, font, VOL_POS, 20, (FcChar8 *)vol_str, strlen(vol_str));

    // Separador
    XftDrawStringUtf8(draw, sep_color, font, VOL_POS + 60, 20, (FcChar8 *)SEP, strlen(SEP));

    // Batería
    char battery_str[32];
    int battery = get_battery_percentage();
    if (battery >= 0) {
        snprintf(battery_str, sizeof(battery_str), "Battery: %d%%", battery);
    } else {
        strcpy(battery_str, "Battery: N/A");
    }
    XftDrawStringUtf8(draw, text_color, font, BATTERY_POS, 20, (FcChar8 *)battery_str, strlen(battery_str));

    // Separador
    XftDrawStringUtf8(draw, sep_color, font, BATTERY_POS + 90, 20, (FcChar8 *)SEP, strlen(SEP));

    // Fecha
    char date_str[32];
    strftime(date_str, sizeof(date_str), "%d-%m-%Y", tm_info);
    XftDrawStringUtf8(draw, text_color, font, DATE_POS, 20, (FcChar8 *)date_str, strlen(date_str));

    // Separador
    XftDrawStringUtf8(draw, sep_color, font, DATE_POS + 80, 20, (FcChar8 *)SEP, strlen(SEP));

    // Hora
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%H:%M", tm_info);
    XftDrawStringUtf8(draw, text_color, font, TIME_POS, 20, (FcChar8 *)time_str, strlen(time_str));
}

int main() {
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        printf("Error: Cannot open display\n");
        return 1;
    }

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    Window bar = XCreateSimpleWindow(display, root, 0, 0, BAR_WIDTH, BAR_HEIGHT, 0,
                                     BlackPixel(display, screen), BG_COLOR);

    set_dock_properties(display, bar);
    XMapWindow(display, bar);

    XftFont *font = XftFontOpen(display, screen,
                                XFT_FAMILY, XftTypeString, "sans",
                                XFT_SIZE, XftTypeDouble, TEXT_SIZE,
                                NULL);
    if (!font) {
        printf("Error: Cannot load font\n");
        XCloseDisplay(display);
        return 1;
    }

    XftDraw *draw = XftDrawCreate(display, bar, DefaultVisual(display, screen), DefaultColormap(display, screen));
    XftColor text_color, sep_color;
    XftColorAllocName(display, DefaultVisual(display, screen), DefaultColormap(display, screen), TEXT_COLOR, &text_color);
    XftColorAllocName(display, DefaultVisual(display, screen), DefaultColormap(display, screen), SEP_COLOR, &sep_color);

    while (1) {
        update_bar(display, bar, draw, &text_color, &sep_color, font);
        XFlush(display);
        sleep(1);
    }

    XftColorFree(display, DefaultVisual(display, screen), DefaultColormap(display, screen), &text_color);
    XftColorFree(display, DefaultVisual(display, screen), DefaultColormap(display, screen), &sep_color);
    XftDrawDestroy(draw);
    XftFontClose(display, font);
    XDestroyWindow(display, bar);
    XCloseDisplay(display);
    return 0;
}
