#ifdef __cplusplus
extern "C" {
#endif

#ifndef CURSED_DRAWING_H
#define CURSED_DRAWING_H

#include <ncurses.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// Window structure with origin, width, height, and depth
typedef struct {
    int x, y;
    int width, height;
    uint_fast32_t z; // Depth
	uint id;
    char* data; // Text to show in the window
    int* dataColor;
    int* dataAttributes;
    char* title;
    int scrollDownOffset;
    int visible;
} Window;

typedef struct {
	uint_fast32_t character;
	int colorPair;
	int attribute;
	//char controlFG[8], controlBG[8]; // Does not work
} Character; 

extern Window* windows;
extern uint numWindows;
extern int focusedWindowIndex; // Initially focused on the first window
extern uint_fast32_t prevMaxTermX, prevMaxTermY;
extern uint_fast32_t maxTermX, maxTermY;
extern char autoScroll;
extern int cmdHistoryWinIndex, cmdInputWinIndex, menuSidebarWinIndex, messageBoxWinIndex, mapWinIndex, ircHistoryWinIndex, ircInputWinIndex;
extern uint sidebarWindowWidth;

int initNcurses();

extern Character **screenBuffer;  // 2D array for the virtual screen buffer

char* read_file_as_string(const char* filename);

int draw_border_to_buffer(uint windowsIndex);

int compare_z(const void* a, const void* b);

int draw_all_windows(); // Adds the window borders to the display buffer

void update_screen(); // Updates the ncurses display

void clear_display_buffer();

void scrollToBottom(uint windowsIndex);

int create_window(int x, int y, int width, int height, char* title);

void resize_windows();

#endif

#ifdef __cplusplus
}
#endif
