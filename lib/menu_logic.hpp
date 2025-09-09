#ifndef MENU_LOGIC_HPP
#define MENU_LOGIC_HPP

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>
#include "irc_logic.hpp"
#include "cursed_drawing.h"

namespace Menus {
	typedef struct {
		uint x, y;
		std::string name;
	} MapMarker;
}

extern int running;
extern std::string menuWindowSidebarText; // Text data for the main sidebar menu
extern int menuSidebarHighlightedIndex;
extern int menuActiveIndex;
extern std::vector<std::string> menuSidebarOptions;
extern std::vector<Menus::MapMarker> mapMarkers;

namespace Menus {
	void inline removeTrailingAsterisk(int index) {
	    char* oldTitle = windows[index].title;
	    size_t len = strlen(oldTitle);
	
	    if (len > 0 && oldTitle[len - 1] == '*') {
	        char* newTitle = new char[len];
	        strncpy(newTitle, oldTitle, len - 1);
	        newTitle[len - 1] = '\0';
	
	        free(windows[index].title);
	        windows[index].title = newTitle;
	    }
	}
	
	void inline addAsteriskToTitle(int index) {
	    char* title = windows[index].title;
	    size_t len = strlen(title);
	
	    char* modifiedTitle = new char[len + 2]; // +1 for '*' +1 for '\0'
	    strcpy(modifiedTitle, title);
	    modifiedTitle[len] = '*';
	    modifiedTitle[len + 1] = '\0';
	
	    free(windows[index].title);
	    windows[index].title = modifiedTitle;
	}
	
	int inline getNextVisibleIndex(int startIndex, int step) {
	    int tempIndex = -1;
	    int i = 1;
	    while (true) {
	        tempIndex = (startIndex + i * step + numWindows) % numWindows;
	        if (windows[tempIndex].visible) break;
	        i++;
	    }
	    return tempIndex;
	}
	
	void inline increaseWindowFocus() {
	    removeTrailingAsterisk(focusedWindowIndex);
	
	    int nextIndex = getNextVisibleIndex(focusedWindowIndex, 1);
	    addAsteriskToTitle(nextIndex);
	
	    focusedWindowIndex = nextIndex;
	}
	
	void inline decreaseWindowFocus() {
	    removeTrailingAsterisk(focusedWindowIndex);
	
	    int prevIndex = getNextVisibleIndex(focusedWindowIndex, -1);
	    addAsteriskToTitle(prevIndex);
	
	    focusedWindowIndex = prevIndex;
	}
	
	void inline changeWindowFocus(int newWindowIndex) {
		removeTrailingAsterisk(focusedWindowIndex);
		
		addAsteriskToTitle(newWindowIndex);
		
		focusedWindowIndex = newWindowIndex;
	}
	
	void switchActiveMenu();
	void initMap();
	void initSidebarMenu(std::string& menuData, int selectedIndex);
	void messageBox(std::string message, std::string title, std::vector<std::string> options, int width, int height);
}

#endif
