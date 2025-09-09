#include "menu_logic.hpp"

int running = 0;
std::string menuWindowSidebar;
int menuSidebarHighlightedIndex = 0;
int menuActiveIndex = 0;
int menuQuitIndex = 3;
std::vector<std::string> menuSidebarOptions;
std::vector<Menus::MapMarker> mapMarkers;

namespace Menus {
	void switchActiveMenu() {
		// Hide all the relevant windows
		windows[cmdInputWinIndex].visible = 0;
		windows[cmdHistoryWinIndex].visible = 0;
		windows[mapWinIndex].visible = 0;
		windows[ircHistoryWinIndex].visible = 0;
		windows[ircInputWinIndex].visible = 0;
		// Also default hide the cursor
		curs_set(0);
		if (menuActiveIndex == 0) { // cmd
			curs_set(1);
			windows[cmdHistoryWinIndex].visible = 1;
			windows[cmdInputWinIndex].visible = 1;
		} else if (menuActiveIndex == 1) { // Map
			windows[mapWinIndex].visible = 1;
			initMap();
		} else if (menuActiveIndex == 2) { // IRC
			curs_set(1);
			if (ircOpened == false) {
				std::string introMessage;
				for (uint_fast32_t i = 0; i < windows[ircHistoryWinIndex].height; i++) {
					introMessage += "\n";
				}
				introMessage += IRCLogic::introMessage;
				windows[ircHistoryWinIndex].data = const_cast<char*>(introMessage.c_str());
			}
			ircOpened = true;
			windows[ircHistoryWinIndex].visible = 1;
			windows[ircInputWinIndex].visible = 1;
			IRCLogic::reconnect(); // Init the connection now (reconnect starts a loop)
		} else if (menuActiveIndex == menuQuitIndex) {
			running = 0; // Quit safely
		}
	}
	
	void initMap() {
		static char* mapText;
		delete[] mapText;
		for (static auto& marker : mapMarkers) {
			
		}
	}
	
	void initSidebarMenu(std::string& menuData, int selectedIndex) {
		menuSidebarOptions = {"Command Window", "Map", "IRC", "Quit"};
	    std::string sidebarText =  "";
	    for (int i = 0; i < menuSidebarOptions.size(); i++) {
			if (selectedIndex == i) {menuSidebarOptions[i] = "^]1b[{" + menuSidebarOptions[i] + "}";} 
	        sidebarText += menuSidebarOptions[i] + "\n \n";
	    }
	    menuData = sidebarText;
	}

	void messageBox(std::string message, std::string title, std::vector<std::string> options, int width, int height) {
	    windows[messageBoxWinIndex].visible = 1;
	    windows[messageBoxWinIndex].x = maxTermX / 2 - width / 2;
	    windows[messageBoxWinIndex].y = maxTermY / 2 - height / 2;
	    windows[messageBoxWinIndex].height = height;
	    windows[messageBoxWinIndex].width = width;
	
	    static char* storedTitle;
	    static char* storedMessage;
	    
		delete[] storedTitle;
	    delete[] storedMessage;
	
	    storedTitle = new char[title.length() + 1];
	    storedMessage = new char[message.length() + 1];
	    
        strcpy(storedTitle, title.c_str());
	    strcpy(storedMessage, message.c_str());
	
	    windows[messageBoxWinIndex].title = storedTitle;
	    windows[messageBoxWinIndex].data = storedMessage;
	    //changeWindowFocus(messageBoxWinIndex);
	}
}
