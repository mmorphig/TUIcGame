#include "cursed_drawing.h"

Window* windows = NULL;
uint numWindows = 0;
int focusedWindowIndex = 0; // Initially focused on the first window
uint_fast32_t prevMaxTermX, prevMaxTermY;
uint_fast32_t maxTermX, maxTermY;
char autoScroll = 1;
int cmdHistoryWinIndex, cmdInputWinIndex, menuSidebarWinIndex, messageBoxWinIndex, mapWinIndex, ircHistoryWinIndex, ircInputWinIndex;

Character **screenBuffer = NULL;  // 2D array for the virtual screen buffer

int initNcurses() {
	initscr();
    noecho();
    curs_set(1);
    keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE); // Make getch() non-blocking
    getmaxyx(stdscr, maxTermY, maxTermX);
    start_color();

	init_pair(0, COLOR_WHITE, COLOR_BLACK);
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
	init_pair(2, COLOR_BLACK, COLOR_RED);
	init_pair(3, COLOR_RED, COLOR_BLACK);
    init_pair(4, COLOR_GREEN, COLOR_BLACK);
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);
    init_pair(6, COLOR_BLUE, COLOR_BLACK);
    init_pair(7, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(8, COLOR_CYAN, COLOR_BLACK);

	// Initialize the screen buffer
    screenBuffer = (Character**)malloc(maxTermY * sizeof(Character*));
	Character emptyCharacter = {.character = ' ', .colorPair = 0};
	for (int i = 0; i < maxTermY; i++) {
	    screenBuffer[i] = (Character*)malloc(maxTermX * sizeof(Character));
	    for (int j = 0; j < maxTermX; j++) {
			screenBuffer[i][j] = emptyCharacter;
			// Fill with spaces, basically clearing the buffer so it's not just reading random memory data
	    }
	}
	
	return 1;	
}

int draw_border_to_buffer(uint windowsIndex) {
    if (windows[windowsIndex].x >= maxTermX || windows[windowsIndex].y >= maxTermY) {
        return 1; // Skip drawing if out of bounds
    }

    char** lines = NULL;
    long long int line_count = 0;

    if (windows[windowsIndex].data) {
        char* data_copy = strdup(windows[windowsIndex].data);
        char* token = strtok(data_copy, "\n");
        while (token) {
            lines = realloc(lines, sizeof(char*) * (line_count + 1));
            lines[line_count++] = strdup(token);
            token = strtok(NULL, "\n");
        }
        free(data_copy);
    }

    // Apply scroll offset to skip lines
    uint scroll_offset = windows[windowsIndex].scrollDownOffset;
    if (scroll_offset >= line_count) scroll_offset = line_count - 1;  // Cap offset
    if (scroll_offset < 0) scroll_offset = 0;

    for (uint_fast32_t i = 0; i < windows[windowsIndex].height; i++) {
        int current_y = windows[windowsIndex].y + i;
        if (current_y >= maxTermY) break;

        int last_char_index = windows[windowsIndex].width - 3;

        for (uint_fast32_t j = 0; j < windows[windowsIndex].width; j++) { // Sraw the rows
            if (windows[windowsIndex].x + j < 0 || windows[windowsIndex].x + j >= maxTermX) continue;

            if (i == 0) { // Draw top border/title
                if (j == 0) {
                    screenBuffer[current_y][windows[windowsIndex].x + j].character = ACS_ULCORNER;
                } else if (j == windows[windowsIndex].width - 1) {
                    screenBuffer[current_y][windows[windowsIndex].x + j].character = ACS_URCORNER;
                } else {
                    screenBuffer[current_y][windows[windowsIndex].x + j].character =
                        (windows[windowsIndex].title && j - 1 < (uint)strlen(windows[windowsIndex].title))
                            ? windows[windowsIndex].title[j - 1]
                            : ACS_HLINE;
                }
            } else if (i == windows[windowsIndex].height - 1) { // Draw bottom border
                if (j == 0) {
                    screenBuffer[current_y][windows[windowsIndex].x + j].character = ACS_LLCORNER;
                } else if (j == windows[windowsIndex].width - 1) {
                    screenBuffer[current_y][windows[windowsIndex].x + j].character = ACS_LRCORNER;
                } else {
                    screenBuffer[current_y][windows[windowsIndex].x + j].character = ACS_HLINE;
                }
            } else { // Draw content or vertical borders
                uint_fast32_t line_index = i - 1 + scroll_offset;
                uint_fast32_t char_index = j - 1;

                if (j == 0 || j == windows[windowsIndex].width - 1) {
                    screenBuffer[current_y][windows[windowsIndex].x + j].character = ACS_VLINE;
                } else if (line_index < line_count && lines[line_index]) {
                    if (char_index < last_char_index && char_index < (uint)strlen(lines[line_index])) { // Draw a data character
                        screenBuffer[current_y][windows[windowsIndex].x + j].character = lines[line_index][char_index];
                        if (windowsIndex <= 2) {
						    // Compute the flat index into dataColor
						    size_t flat_index = 0;
							for (uint_fast32_t li = 0; li < line_index; li++) {
							    if (lines[li]) flat_index += strlen(lines[li]) + 1; // +1 to account for '\n'
							}
							flat_index += char_index;
							
						    screenBuffer[current_y][windows[windowsIndex].x + j].colorPair = windows[windowsIndex].dataColor[flat_index];
						    screenBuffer[current_y][windows[windowsIndex].x + j].attribute = windows[windowsIndex].dataAttributes[flat_index];
						}
                    } else if (char_index == last_char_index && (uint)strlen(lines[line_index]) > last_char_index) {
                        screenBuffer[current_y][windows[windowsIndex].x + j].character = '>';
                        screenBuffer[current_y][windows[windowsIndex].x + j - 1].character = ' ';
                    } else {
                        screenBuffer[current_y][windows[windowsIndex].x + j].character = ' ';
                    }
                } else {
                    screenBuffer[current_y][windows[windowsIndex].x + j].character = ' ';
                }
            }
        }
    }

    // Free allocated memory for lines
    for (int i = 0; i < line_count; i++) free(lines[i]);
    free(lines);

    return 1;
}

// Compare function for qsort to sort by z order
int compare_z(const void* a, const void* b) {
    Window* winA = (Window*)a;
    Window* winB = (Window*)b;
    return winA->z - winB->z; // Ascending order
}

int draw_all_windows() {
	// Sort windows by z order before drawing
    qsort(windows, numWindows, sizeof(Window), compare_z);
    
    for (unsigned char i = 0; i < numWindows; i++) {
		if (!windows[i].visible) continue;
        if (!draw_border_to_buffer(i)) return 0;
    }
    return 1;
}

void update_screen() {
    // Update only the parts that have changed
    for (uint i = 0; i < maxTermY; ++i) {
	    Character* row = screenBuffer[i];
	    int y = i;
	    int x = 0;
	    move(y, 0); // Move once per line
	    for (uint j = 0; j < maxTermX; ++j) {
			attron(COLOR_PAIR(row[j].colorPair));
			addch(row[j].character);
		    attroff(COLOR_PAIR(row[j].colorPair));
	    }
	}
    refresh();
}

void clear_display_buffer() {
	Character emptyCharacter = {.character = ' ', .colorPair = 0};
	for (int i = 0; i < maxTermY; i++) {
	    for (int j = 0; j < maxTermX; j++) {
			screenBuffer[i][j] = emptyCharacter;
			// Fill with spaces, basically clearing the buffer so it's not just reading random memory data
	    }
	}
}

void scrollToBottom(uint windowsIndex) {
    if (windowsIndex >= numWindows) return;  // Invalid window index

    // Get the number of lines in the window's data
    int line_count = 0;
    if (windows[windowsIndex].data) {
        char* data_copy = strdup(windows[windowsIndex].data);
        char* token = strtok(data_copy, "\n");
        while (token) {
            line_count++;
            token = strtok(NULL, "\n");
        }
        free(data_copy);
    }

    // Set the scroll offset to the bottom, effectively scrolling the window to the bottom
    windows[windowsIndex].scrollDownOffset = line_count - windows[windowsIndex].height + 2;
    if (windows[windowsIndex].scrollDownOffset < 0) {
        windows[windowsIndex].scrollDownOffset = 0;  // Ensure it doesn't go negative
    }
}

int create_window(int x, int y, int width, int height, char* title) {
    numWindows++;

    // Reallocate memory for the new window array
    windows = (Window*)realloc(windows, numWindows * sizeof(Window));
    if (!windows) {
        return 0; // Memory allocation failed
    }

    // Allocate and copy the title string
    char* titleCopy = NULL;
    if (title) {
        size_t titleLen = strlen(title);
        titleCopy = (char*)malloc(titleLen + 1); // +1 for null terminator
        if (!titleCopy) {
            return 0; // Allocation failed
        }
        strcpy(titleCopy, title);
    }

    // Set z based on the number of windows
    int z = numWindows;
    unsigned int id = numWindows;

    windows[numWindows - 1] = (Window){x, y, width, height, z, id, NULL, NULL, NULL, titleCopy, 0, 1};

    return 1; // Success
}

void resize_windows() {
    int oldMaxTermY = maxTermY; // Save previous height
    getmaxyx(stdscr, maxTermY, maxTermX);

    // Reallocate screenBuffer pointer array
    screenBuffer = (Character**)realloc(screenBuffer, maxTermY * sizeof(Character*));
    if (!screenBuffer) {
        // Handle allocation failure
        endwin();
        fprintf(stderr, "Failed to reallocate screenBuffer rows\n");
        exit(1);
    }

    for (int i = 0; i < maxTermY; i++) {
        if (i < oldMaxTermY && screenBuffer[i]) {
            screenBuffer[i] = (Character*)realloc(screenBuffer[i], maxTermX * sizeof(Character));
        } else {
            screenBuffer[i] = (Character*)malloc(maxTermX * sizeof(Character));
        }

        if (!screenBuffer[i]) {
            // Handle allocation failure
            endwin();
            fprintf(stderr, "Failed to allocate screenBuffer[%d]\n", i);
            exit(1);
        }

        // Clear the line
        for (int j = 0; j < maxTermX; j++) {
            screenBuffer[i][j].character = 0x20;
	    }
	}
}
