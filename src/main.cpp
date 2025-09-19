#include <algorithm>
#include <iostream>
#include <string>
#include <sstream>
#include <limits>
#include <vector>
#include <ncurses.h>
#include <cstdlib>
#include <cstring>
#include "../lib/cursed_drawing.h"
#include "../lib/code_reading.hpp"
#include "../lib/cmd_logic.hpp"
#include "../lib/menu_logic.hpp"
#include "../lib/game_elements.hpp"
#include "../lib/irc_logic.hpp"
#include "../lib/txt_reader.h"
#include "../lib/txt_writer.h"

int debug = 0;

int cmdCursorPos = 0;
int ircCursorPos = 0;

bool userInput() {
    int key = getch(); // Must be int, not char, to handle special keys
	
    if (key != ERR) {
        switch (key) {
            case '\n': {// Enter
				if (focusedWindowIndex == cmdInputWinIndex) {
	                fileLog(cmdUserPrompt.c_str());
	                fileLog(cmdText.c_str());
	                CmdStuff::Reading::interpretCmdText(cmdText);
	                writeLineToFile(cmdHistoryFile.c_str(), cmdText.c_str());
	                cmdText.clear();
	                cmdCursorPos = 0;
                } else if (focusedWindowIndex == ircInputWinIndex) {
	                IRCLogic::interpretIRCInput(ircText);
	                ircText.clear();
	                ircCursorPos = 0;
                } else if (focusedWindowIndex == menuSidebarWinIndex) {
					menuActiveIndex = menuSidebarHighlightedIndex;
					Menus::switchActiveMenu();
				} else if (focusedWindowIndex == messageBoxWinIndex) {
					windows[3].visible = 0;
					Menus::decreaseWindowFocus();
				} 
                break;
            }
            case KEY_BACKSPACE:
            case 127: // Some systems send 127
	            if (focusedWindowIndex == cmdInputWinIndex) {
	                if (cmdCursorPos > 0) {
	                    cmdText.erase(cmdCursorPos - 1, 1);
	                    cmdCursorPos--;
	                }
				} else if (focusedWindowIndex == ircInputWinIndex) {
					if (ircCursorPos > 0) {
	                    ircText.erase(ircCursorPos - 1, 1);
	                    ircCursorPos--;
	                }
				}
                break;
            case KEY_DC: {
	            if (focusedWindowIndex != cmdInputWinIndex) break;
                if (cmdCursorPos < cmdText.size()) {
                    cmdText.erase(cmdCursorPos, 1);
                }
                break;
            }

            case KEY_LEFT: {
				if (focusedWindowIndex == cmdInputWinIndex) {
	                if (cmdCursorPos > 0) cmdCursorPos--;
                } else if (focusedWindowIndex == ircInputWinIndex) {
	                if (ircCursorPos > 0) ircCursorPos--;
                }
                break;
            }
            case KEY_RIGHT: {
	            if (focusedWindowIndex == cmdInputWinIndex) {
	                if (cmdCursorPos > 0) cmdCursorPos++;
                } else if (focusedWindowIndex == ircInputWinIndex) {
	                if (ircCursorPos > 0) ircCursorPos++;
                }
                break;
            }
            
            case KEY_UP: {
	            if (focusedWindowIndex == cmdInputWinIndex) { // In command prompt
		            cmdHistoryFileLength = countLinesInFile(cmdHistoryFile.c_str());
					if (cmdHistoryFileLength == 0) break;
				
					if (cmdHistoryOffset == 0) {
						cmdTextCache = cmdText; // Cache current input before browsing history
					}
					if (cmdHistoryOffset < cmdHistoryFileLength) {
						cmdHistoryOffset++;
						int index = cmdHistoryFileLength - cmdHistoryOffset;
						if (index >= 0 && index < cmdHistoryFileLength) {
							cmdText = readLineFromFile(cmdHistoryFile.c_str(), index + 1);
							char* tempText = readLineFromFile(cmdHistoryFile.c_str(), index + 1);
							if (tempText) {
							    cmdText = tempText;
							    free(tempText);  // Prevent memory leak
							} else { cmdText.clear(); }
							cmdCursorPos = cmdText.size();
						}
					}
					break;
				} else if (focusedWindowIndex == menuSidebarWinIndex) { // Sidebar menu
					menuSidebarHighlightedIndex = (menuSidebarHighlightedIndex - 1) % menuSidebarOptions.size();
					break;
				} else if (focusedWindowIndex == stgMenuWinIndex) { break; }
				break;
			}
			
			case KEY_DOWN: {
				if (focusedWindowIndex == cmdInputWinIndex) { // In command prompt
					cmdHistoryFileLength = countLinesInFile(cmdHistoryFile.c_str());
					if (cmdHistoryOffset == 0) break;
				
					cmdHistoryOffset--;
					if (cmdHistoryOffset == 0) {
						cmdText = cmdTextCache; // Restore cached input
					} else {
						int index = cmdHistoryFileLength - cmdHistoryOffset;
						if (index >= 0 && index < cmdHistoryFileLength) {
							cmdText = readLineFromFile(cmdHistoryFile.c_str(), index + 1);
							char* tempText = readLineFromFile(cmdHistoryFile.c_str(), index + 1);
							if (tempText) {
							    cmdText = tempText;
							    free(tempText);  // Prevent memory leak
							} else { cmdText.clear(); }
						}
					}
					cmdCursorPos = cmdText.size();
					break;
				} else if (focusedWindowIndex == menuSidebarWinIndex) { // Sidebar menu
					menuSidebarHighlightedIndex = (menuSidebarHighlightedIndex + 1) % menuSidebarOptions.size();
					break;
				} else if (focusedWindowIndex == stgMenuWinIndex) { break; }
				break;
			}
			
			case 0x09: {
			    Menus::increaseWindowFocus();
			    break;
			}
			
            default: {
                if (key >= 32 && key <= 126) { // Printable ASCII
					if (focusedWindowIndex == cmdInputWinIndex) {
	                    cmdText.insert(cmdCursorPos, 1, static_cast<char>(key));
	                    cmdCursorPos++;
                    } else if (focusedWindowIndex == ircInputWinIndex) {
						ircText.insert(ircCursorPos, 1, static_cast<char>(key));
						ircCursorPos++;
					}
                }
                break;
			}
        }
    }
    return true;
}

void processEscapeSequences(std::string& text, std::vector<int>& colorData, std::vector<int>& attributeData) {
    std::string result;
    std::vector<int> colorResult, attributeResult;

    size_t i = 0;
    int currentColor = 0;
    int currentAttr = 0;

    while (i < text.length()) {
        // Look for ^]... escape sequence
        if (text[i] == '^' && i + 1 < text.length() && text[i + 1] == ']') {
            size_t j = i + 2;
            std::string numberStr;
            bool isBold = false;

            // Optional digits for colour
            while (j < text.length() && isdigit(text[j])) {
                numberStr += text[j];
                ++j;
            }

            // Optional bold flag
            if (j < text.length() && text[j] == 'b') {
                isBold = true;
                ++j;
            }

            // Expecting [ followed by {text}]
            if (j < text.length() && text[j] == '[') {
                ++j; // Skip '['

                if (j < text.length() && text[j] == '{') {
                    ++j; // Skip '{'
                    size_t contentStart = j;

                    // Find closing '}'
                    while (j < text.length() && text[j] != '}') {
                        ++j;
                    }

                    if (j < text.length() && text[j] == '}') {
                        // Valid escape sequence found
                        int colorValue = numberStr.empty() ? currentColor : std::stoi(numberStr);
                        int attrValue = isBold ? A_BOLD : 0;

                        for (size_t k = contentStart; k < j; ++k) {
                            result += text[k];
                            colorResult.push_back(colorValue);
                            attributeResult.push_back(attrValue);
                        }

                        ++j;    // Skip '}'
                        i = j;  // Move past escape sequence
                        continue;
                    }
                }
            }
        }

        // Handle double newline
        if (text[i] == '\n' && i + 2 < text.length() && text[i + 1] == '\n' && !(text[i + 2] == ' ')) {
            result += "\n \n";
            colorResult.push_back(currentColor);
            colorResult.push_back(currentColor);
            attributeResult.push_back(currentAttr);
            attributeResult.push_back(currentAttr);
            i += 1;
            continue;
        }

        // Handle tabs
        while (text[i] == '\t') {
            result += "  ";
            colorResult.push_back(currentColor);
            colorResult.push_back(currentColor);
            attributeResult.push_back(currentAttr);
            attributeResult.push_back(currentAttr);
            ++i;
        }

        // Normal character
        result += text[i];
        colorResult.push_back(currentColor);
        attributeResult.push_back(currentAttr);
        ++i;
    }

    text = result;
    colorData = colorResult;
    attributeData = attributeResult;
}

int* vectorIntToIntArray(const std::vector<int>& vec) {
    if (vec.empty()) return nullptr;

    int* cArray = new int[vec.size()];
    std::copy(vec.begin(), vec.end(), cArray);
    return cArray;
}

int main(int argc, char* argv[]) {
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-g") == 0 && i < argc) { // no + 1 because there is no argument
			debug = 1;
			std::cout << "UwU\n\n";
		} else {
			printf("Unknown or incomplete option: %s\n", argv[i]);
			return 1; // No mercy
		}
	}
	
	system("mkdir -p ./save");
	int newSave = 1;
	
	while (cmdName == "") {
		std::string keep;
		std::cout << "Name: ";
		getline(std::cin, cmdName);
		if (cmdName == "root") {
			cmdName = "";
			std::cout << "Name cannot be \"root\"\n";
		} else {
			std::cout << "Is this name correct?\n " << cmdName << "\n";
			std::cin >> keep;
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			std::transform(keep.begin(), keep.end(), keep.begin(),
				           [](unsigned char c) { return std::tolower(c); });
			if (keep[0] == 'n') {
				cmdName = "";
			}
		}
	}
	std::cout << "\nloading...\n";
	
	rootDir = "./save/" + cmdName + "/root/";
    homeDir = "./save/" + cmdName + "/root/home/" + cmdName + "/"; // Put the slash there so it does not need to be put in when used
	
	cmdHomeDir = "/home/" + cmdName + "/";
	cmdCurrentDirectory = "/home/" + cmdName + "/";
	cmdCurrentDisplayDirectory = "~/";
	std::string hostnameFile = rootDir + "etc/hostname";
	cmdWindowFile = rootDir + "etc/" + cmdName + "TerminalHistory.txt";
	cmdHistoryFile = homeDir + ".bash_history";
	ircWindowFile = homeDir + ".cache/simpleIRC/history.txt";
	ircConnectionTempFile = rootDir + "tmp/ircConnected.tmp.txt";
	std::ifstream testFile;
	
	// Test if the save exists already
	if (debug) { // If the user does not have a hostname, then it's a new save
		testFile = std::ifstream(hostnameFile + "g"); // Set to hostnameg for development, so it is always a new save, change when needed/release
	} else {
		testFile = std::ifstream(hostnameFile); // Actual hostname
	}
    if (testFile.is_open()) { // TODO: maybe make the help message have a location in the player's root dir
		cmdHelpMessageFilepath = "resources/helpmsg.txt";
		newSave = 0; // newSave is set to 0, as this is a save that was already created
		// Just to make sure, clear proc and tmp
		std::string clearTempCommand = "rm -rf ./save/" + cmdName + "/root/proc/*}";
		system(clearTempCommand.c_str());
		clearTempCommand = "rm -rf ./save/" + cmdName + "/root/tmp/*}";
		system(clearTempCommand.c_str());
		char* tempHostname = readLineFromFile(hostnameFile.c_str(), 1);
		if (tempHostname) {
		    cmdHostname = tempHostname;
		    free(tempHostname);  // Prevent memory leak
		} else { cmdHostname.clear(); }
		emptyFile(cmdWindowFile.c_str());
		emptyFile(cmdHistoryFile.c_str());
    } else {
		cmdHelpMessageFilepath = "resources/helpmsg.txt";
		createFile(cmdWindowFile.c_str());
		
		// create the home dir, the save directory is named the same, it's simpler
		std::string command = "mkdir -p " + homeDir; 
		system(command.c_str());
		
		std::string newSaveCommand = "mkdir -p " + rootDir + "{etc,run,tmp,var,root,proc,usr/bin}";
		system(newSaveCommand.c_str()); // Create a bunch of directories in the root folder
		
		cmdHostname = "EpicHaxComputer"; // Default hostname
		createFile(hostnameFile.c_str());
		writeLineToFile(hostnameFile.c_str(), cmdHostname.c_str());
		
		createFile(cmdHistoryFile.c_str());
		createFile(ircWindowFile.c_str());
		createFile(ircConnectionTempFile.c_str());
	}
    testFile.close();
    // Test if tree is installed, then run it
    testFile = std::ifstream("/usr/bin/tree");
    if (testFile.is_open()) {
		std::cout << "\nCurrent save's root directory layout:\n";
		std::string treeCommand = "tree -a ./save/" + cmdName + "/root/";
		system(treeCommand.c_str());
		std::cout << "\n\n";
    }
    testFile.close();
    
    std::string cppLogDir = "./save/" + cmdName + "/"; // std::string version of the log dir
    logDir = const_cast<char*>(cppLogDir.c_str());
    std::string cppLogFile = "log.txt"; // std::string version of the log file
    logFile = const_cast<char*>(cppLogFile.c_str());
    std::string resetLogLocation = "rm -f " + cppLogDir + cppLogFile;
    system(resetLogLocation.c_str()); // Clear the log
    cmdUserPrompt = "^]4[{" + cmdName + "}@" + "^]6[{" + cmdHostname + "} ^]4[{" + cmdCurrentDisplayDirectory + "} $ ";
    ircUserPrompt = ircNickname + " -> ";
    
    std::vector<int> cmdHistWinColor, cmdPromptWinColor, ircHistWinColor, ircPromptWinColor, sidebarWinColor; // Temporary 
    std::vector<int> cmdHistWinAttribute, cmdPromptWinAttribute, ircHistWinAttribute, ircPromptWinAttribute, sidebarWinAttribute;
    
    std::cout << "\nLoading done!\n";
    napms(2000);
    
    system("clear");
    
    initNcurses();
    
	// Initialize all the windows
    cmdHistoryWinIndex = numWindows; // 0
    create_window(sidebarWindowWidth - 1, 1, maxTermX - sidebarWindowWidth - 2, maxTermY - 4, (char*)"Command Window");

    cmdInputWinIndex = numWindows; // 1
    create_window(sidebarWindowWidth - 1, maxTermY - 4, maxTermX - sidebarWindowWidth - 2, 3, (char*)"Cmd*"); // Starts active so it starts with the asterisk

    menuSidebarWinIndex = numWindows; // 2
    create_window(1, 1, sidebarWindowWidth, maxTermY - 4, (char*)"Menu");

    messageBoxWinIndex = numWindows; // 3
    create_window(1, 1, 20, 20, (char*)"impossible.text");

    mapWinIndex = numWindows; // 4
    create_window(sidebarWindowWidth - 1, 1, maxTermX - sidebarWindowWidth - 2, maxTermY - 2, (char*)"Map");

    ircHistoryWinIndex = numWindows; // 5
    create_window(sidebarWindowWidth - 1, 1, maxTermX - sidebarWindowWidth - 2, maxTermY - 4, (char*)"IRC Chat");

    ircInputWinIndex = numWindows; // 6
    create_window(sidebarWindowWidth - 1, maxTermY - 4, maxTermX - sidebarWindowWidth - 2, 3, (char*)"");

	stgMenuWinIndex = numWindows; // 7
	create_window(sidebarWindowWidth - 1, 1, maxTermX - sidebarWindowWidth - 2, maxTermY - 2, (char*)"Settings");

    windows[messageBoxWinIndex].visible = 0;
    windows[mapWinIndex].visible = 0;
    windows[ircHistoryWinIndex].visible = 0;
    windows[ircInputWinIndex].visible = 0;
	windows[stgMenuWinIndex].visible = 0;
    std::string menuData;
    Menus::initSidebarMenu(menuData, menuSidebarHighlightedIndex);
    windows[menuSidebarWinIndex].data = const_cast<char*>(menuData.c_str());
    focusedWindowIndex = 1;
    
	std::string cmdPromptWinData, ircPromptWinData;
	
	running = 1;

	while (running) {
		// Update cmd windows
		std::string cmdHistWinData = InterpretCode::readSourceFile(cmdWindowFile);
		delete[] windows[cmdHistoryWinIndex].dataColor;
		delete[] windows[cmdHistoryWinIndex].dataAttributes;
		processEscapeSequences(cmdHistWinData, cmdHistWinColor, cmdHistWinAttribute);
		windows[cmdHistoryWinIndex].data = const_cast<char*>(cmdHistWinData.c_str());
		windows[cmdHistoryWinIndex].dataColor = vectorIntToIntArray(cmdHistWinColor);
		windows[cmdHistoryWinIndex].dataAttributes = vectorIntToIntArray(cmdHistWinColor);
		
		cmdPromptWinData = cmdUserPrompt + cmdText;
		delete[] windows[cmdInputWinIndex].dataColor;
		delete[] windows[cmdInputWinIndex].dataAttributes;
		processEscapeSequences(cmdPromptWinData, cmdPromptWinColor, cmdPromptWinAttribute);
		windows[cmdInputWinIndex].data = const_cast<char*>(cmdPromptWinData.c_str());
		windows[cmdInputWinIndex].dataColor = vectorIntToIntArray(cmdPromptWinColor);
		windows[cmdInputWinIndex].dataAttributes = vectorIntToIntArray(cmdPromptWinAttribute);
			
		// Update irc windows
		std::string ircHistWinData = InterpretCode::readSourceFile(ircWindowFile);
		delete[] windows[ircHistoryWinIndex].dataColor;
		delete[] windows[ircHistoryWinIndex].dataAttributes;
		processEscapeSequences(ircHistWinData, ircHistWinColor, ircHistWinAttribute);
		windows[ircHistoryWinIndex].data = const_cast<char*>(ircHistWinData.c_str());
		windows[ircHistoryWinIndex].dataColor = vectorIntToIntArray(ircHistWinColor);
		windows[ircHistoryWinIndex].dataAttributes = vectorIntToIntArray(ircHistWinColor);
		
		ircPromptWinData = ircUserPrompt + ircText;
		delete[] windows[ircInputWinIndex].dataColor;
		delete[] windows[ircInputWinIndex].dataAttributes;
		processEscapeSequences(ircPromptWinData, ircPromptWinColor, ircPromptWinAttribute);
		windows[ircInputWinIndex].data = const_cast<char*>(ircPromptWinData.c_str());
		windows[ircInputWinIndex].dataColor = vectorIntToIntArray(ircPromptWinColor);
		windows[ircInputWinIndex].dataAttributes = vectorIntToIntArray(ircPromptWinAttribute);
		
		// Update menu sidebar window
		Menus::initSidebarMenu(menuData, menuSidebarHighlightedIndex);
		std::string sidebarWinData = menuData;
		delete[] windows[menuSidebarWinIndex].dataColor;
		delete[] windows[menuSidebarWinIndex].dataAttributes;
		processEscapeSequences(sidebarWinData, sidebarWinColor, sidebarWinAttribute);
		windows[menuSidebarWinIndex].data = const_cast<char*>(sidebarWinData.c_str());
		windows[menuSidebarWinIndex].dataColor = vectorIntToIntArray(sidebarWinColor);
		windows[menuSidebarWinIndex].dataAttributes = vectorIntToIntArray(sidebarWinColor);
		
		scrollToBottom(cmdHistoryWinIndex);
		scrollToBottom(ircHistoryWinIndex);
		
		clear_display_buffer();
		for (uint_fast32_t i = 0; i < numWindows; i++) {
			if (!windows[i].visible) continue;
	        draw_border_to_buffer(i);
	    }
		for (uint i = 0; i < maxTermY; ++i) {
		    Character* row = screenBuffer[i];
		    int y = i;
		    int x = 0;
		    move(y, 0); // Move once per line
		    for (uint j = 0; j < maxTermX; ++j) {
				attron(row[j].attribute);
				attron(COLOR_PAIR(row[j].colorPair));
				addch(row[j].character);
			    attroff(COLOR_PAIR(row[j].colorPair));
			    attroff(row[j].attribute);
		    }
		}
		if (menuActiveIndex == 0) {
			move(windows[cmdInputWinIndex].y + 1, windows[cmdInputWinIndex].x + cmdPromptWinData.length() - cmdText.length() + 1 + cmdCursorPos); // Move the cursor to where it should be in the input window
		} else {
			move(windows[ircInputWinIndex].y + 1, windows[ircInputWinIndex].x + ircPromptWinData.length() - ircText.length() + 1 + ircCursorPos); // Move the cursor to where it should be in the input window
		}

		if (!ircConnected && windows[ircInputWinIndex].visible == 1) {
			IRCLogic::reconnect();
		}

	    napms(16); // This is probably fine because this is not super likely to lag anyway
	    resize_windows();
		
		for (unsigned char i = 0; i < numWindows; i++) {
			// Resize and move windows to fit within terminal bounds
			if (i == menuSidebarWinIndex) {
				// menu sidebar, full height with fixed width
				windows[i].width = sidebarWindowWidth;
				windows[i].height = maxTermY - windowPaddingYDouble;
				windows[i].x = windowPaddingX;
				windows[i].y = windowPaddingY;
			} else if (i == cmdHistoryWinIndex) { // hardcoding go brrr
				// cmd history window, full display except the menu on the side and a bit removed from the bottom
				windows[i].width = maxTermX - windows[menuSidebarWinIndex].width - windowPaddingXDouble;
				windows[i].height = maxTermY - inputWindowHeight - windowPaddingYDouble + 1;
				windows[i].x = windowPaddingX + sidebarWindowWidth;
				windows[i].y = windowPaddingY;
			} else if (i == cmdInputWinIndex) {
				// cmd input window, thin window on bottom of history, top of this overlaps the bottom of the history
				windows[i].width = maxTermX - windows[menuSidebarWinIndex].width - windowPaddingXDouble;
				windows[i].height = inputWindowHeight;
				windows[i].x = windowPaddingX + sidebarWindowWidth;
				windows[i].y = maxTermY - inputWindowHeight - windowPaddingY;
			} else if (i == mapWinIndex) {
				// map, same as cmd history, but without the padding for the input
				windows[i].width = maxTermX - windows[menuSidebarWinIndex].width - windowPaddingXDouble;
				windows[i].height = maxTermY - windowPaddingYDouble;
				windows[i].x = windowPaddingX + sidebarWindowWidth;
				windows[i].y = windowPaddingY;
			} else if (i == ircHistoryWinIndex) {
				// irc history, same as cmd history
				windows[i].width = maxTermX - windows[menuSidebarWinIndex].width - windowPaddingXDouble;
				windows[i].height = maxTermY - inputWindowHeight - windowPaddingYDouble + 1;
				windows[i].x = windowPaddingX + sidebarWindowWidth;
				windows[i].y = windowPaddingY;
			} else if (i == ircInputWinIndex) {
				// irc input, same as cmd input
				windows[i].width = maxTermX - windows[menuSidebarWinIndex].width - windowPaddingXDouble;
				windows[i].height = inputWindowHeight;
				windows[i].x = windowPaddingX + sidebarWindowWidth;
				windows[i].y = maxTermY - inputWindowHeight - windowPaddingY;
			} else if (i == stgMenuWinIndex) {
				// settings, same as map
				windows[i].width = maxTermX - windows[menuSidebarWinIndex].width - windowPaddingXDouble;
				windows[i].height = maxTermY - windowPaddingYDouble;
				windows[i].x = windowPaddingX + sidebarWindowWidth;
				windows[i].y = windowPaddingY;
			}
		}

	    refresh();
	    userInput();
	}
	
	IRCLogic::closeConnection();
	
	endwin();
	return 0;
}
