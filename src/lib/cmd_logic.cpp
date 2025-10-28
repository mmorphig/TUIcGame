#include "cmd_logic.hpp"

std::string rootDir; // root and home dir in-game, not for the user's actual system
std::string homeDir;
std::string cmdName;
std::string cmdHomeDir; // Directory for the in-game users home, used to remove a bunch of "/home/" + name
std::string cmdText; // Text the user has inputted into the cmd
std::string cmdCurrentDirectory; // current working directory of the cmd
std::string cmdCurrentDisplayDirectory; // current working directory of the cmd but to show the user
std::string cmdUserPrompt; // "name@hostname cmdCurrentDirectory $ "
std::string cmdHostname; // In-game system hostname
std::string cmdWindowFile; // File that holds the data for the main window, which acts as the stdout in-game
std::string cmdHistoryFile; // bash history in-game
int cmdHistoryOffset; // Offset for the currento position in the history file
int cmdHistoryFileLength;
std::string cmdTextCache; // Holds the current cmdText in cache while cmdText is what the user sees from the history file
std::string cmdHelpMessageFilepath;

namespace CmdStuff {
	// Actual implementation of the interpreter
	namespace Reading {
		// Simple string splitting function
		std::vector<std::string> split(const std::string& str) {
			std::istringstream iss(str);
			std::vector<std::string> tokens;
			std::string token;
			while (iss >> token) tokens.push_back(token);
			return tokens;
		}	
		void interpretCmdText(std::string cmdTextIn) {
			cmdHistoryOffset = 0; // make sure the offset is reset

		    auto tokens = split(cmdTextIn);
		    if (tokens.empty()) return;
		
		    std::string outputFile = cmdWindowFile; // Default output
		    bool append = true;
		
		    // Filter out redirection from tokens
		    std::vector<std::string> cleanedTokens;
		    for (size_t i = 0; i < tokens.size(); ++i) {
		        if (tokens[i] == ">" || tokens[i] == ">>") {
		            if (i + 1 < tokens.size()) {
		                outputFile = resolvePathFilesystem(tokens[i + 1]);
		                append = (tokens[i] == ">>");
		                break; // Stop parsing at redirection
		            }
		        } else {
		            cleanedTokens.push_back(tokens[i]);
		        }
		    }
		
		    if (cleanedTokens.empty()) return;
		
		    int argc = static_cast<int>(cleanedTokens.size());
		    std::vector<char*> argv(argc);
		    for (int i = 0; i < argc; ++i) {
		        argv[i] = &cleanedTokens[i][0];
		    }
		
		    std::string cmd = cleanedTokens[0];
		
		    // Add the original input line to output history
		    std::string cmdTextToFile = cmdUserPrompt + cmdTextIn;
		    writeLineToFile(cmdWindowFile.c_str(), cmdTextToFile.c_str());
		
		    // Dispatch commands
		    if (cmd == "mkdir") {
		        if (argc > 1) Running::cmdMkdir(argv[1], outputFile, append);
		    } else if (cmd == "cat") {
		        if (argc > 1) Running::cmdCat(argv[1], outputFile, append);
		    } else if (cmd == "clear") {
		        Running::cmdClear();
		    } else if (cmd == "cd") {
		        if (argc > 1) Running::cmdCd(argv[1], outputFile, append);
		    } else if (cmd == "ls") {
		        //std::string arg = (argc > 1) ? argv[1] : "";
		        Running::cmdLs(argc, argv, outputFile, append);
		    } else if (cmd == "rmdir") {
		        if (argc > 1) Running::cmdRmdir(argv[1], outputFile, append);
		    } else if (cmd == "rm") {
		        Running::cmdRm(argc, argv, outputFile, append);
		    } else if (cmd == "touch") {
		        Running::cmdTouch(argc, argv, outputFile, append);
		    } else if (cmd == "run") {
		        Running::cmdRun(argv[1], outputFile, append);
		    } else if (cmd == "help") {
		        Running::cmdHelp(outputFile, append);
		    } else if (cmd == "echo") {
		        Running::cmdEcho(argv[1], outputFile, append);
		    } else if (cmd == "reset") {
		        Running::cmdReset(outputFile, append);
		    } else if (cmd == "sudo") { // Not an actual command, but someone might try to use it anyway
		        Running::cmdEcho("Unknown command: sudo \nsudo is not needed", outputFile, append);
		        //Menus::messageBox("Task failed successfully", "Error", std::vector<std::string>{"null"}, 50, 10); // Broken rn, I don't care about it
		    } else {
		        std::string cmdError = "Unknown command: " + cmd;
		        writeLineToFile(outputFile.c_str(), cmdError.c_str());
		    }

			// Debug commands, shows "Unknown command: cmd" but works
			if (cmd == "sidebar-resize") {
				if (argc > 1) Running::cmdSidebarWidth(atoi(argv[1]), outputFile, append);
			} else if (cmd == "padding-resize") {
				if (argc > 1) Running::cmdNewPadding(atoi(argv[1]), outputFile, append);
			} else if (cmd == "save-settings") {
				Running::cmdSaveSettings(outputFile, append);
			}
		}
	}
	
	namespace Running { // all of the comamnds, extra helper functions are inlined in the parent namespace
		void cmdHelp(std::string outputFile, bool append) {
			std::ifstream in(cmdHelpMessageFilepath);
        	if (!in) throw std::runtime_error("Cannot open source file.");
        	std::stringstream buffer;
        	buffer << in.rdbuf();

			cmdEcho(buffer.str(), outputFile, append);
		}
		void cmdRun(std::string file, std::string outputFile, bool append) {
			InterpretCode::run(resolvePathFilesystem(file), homeDir, outputFile, append);
		}
		void cmdCat(std::string file, std::string outputFile, bool append) {
			if (checkPathType(resolvePathFilesystem(file)) == 0) { // If it's a file
				cmdEcho(readFile(resolvePathFilesystem(file)), outputFile, append);
			} else if (checkPathType(resolvePathFilesystem(file)) == 1) {
				cmdEcho("Not a file: " + resolveDirPath(file, 1), outputFile, append);
			} else {
				cmdEcho("File does not exist: " + resolveFilePath(file, 1), outputFile, append);
			}
		}
		void cmdReset(std::string outputFile, bool append) { // Re-does all the initialization and clears
			cmdHomeDir = "/home/" + cmdName + "/";
			cmdCurrentDirectory = "/home/" + cmdName + "/";
			cmdCurrentDisplayDirectory = "~/";
			std::string hostnameFile = rootDir + "etc/hostname";
			cmdHostname = readLineFromFile(hostnameFile.c_str(), 1);
			cmdWindowFile = rootDir + "etc/" + cmdName + "TerminalHistory.txt";
			cmdHistoryFile = homeDir + ".bash_history";
		    cmdUserPrompt = "^]4[{" + cmdName + "}@" + "^]6[{" + cmdHostname + "} ^]4[{" + cmdCurrentDisplayDirectory + "} $ ";
		    cmdClear();
		}
		void cmdMkdir(std::string dir, std::string outputFile, bool append) {
		    std::string realPath = resolvePathFilesystem(dir);
		
		    if (checkPathType(realPath) != -1) {
		        cmdEcho("Directory or file already exists: " + resolveDirPath(dir, 1), outputFile, append);
		        return;
		    }
		
		    int status = mkdir(realPath.c_str(), 0755);
		    if (status == 0) {
		        cmdEcho("Directory created: " + resolveDirPath(dir, 1), outputFile, append);
		    } else {
		        cmdEcho("Failed to create directory: " + resolveDirPath(dir, 1) + " (errno: " + std::to_string(errno) + ")", outputFile, append);
		    }
		}
		void cmdCd(std::string dir, std::string outputFile, bool append) {
			// Start with the current virtual path, strip leading ~
			std::string virtualPath = cmdCurrentDirectory;
			if (!virtualPath.empty() && virtualPath.front() == '~') {
				virtualPath = virtualPath.substr(1);
			}
			
			// Break into path parts
			std::vector<std::string> parts;
			std::stringstream ss(virtualPath);
			std::string segment;
			while (std::getline(ss, segment, '/')) {
				if (!segment.empty()) {
					parts.push_back(segment);
				}
			}
			
			// Apply the cd argument (dir)
			std::stringstream input(dir);
			while (std::getline(input, segment, '/')) {
				if (segment == "..") {
					if (!parts.empty()) parts.pop_back();  // Move one level back
				} else if (segment != "." && !segment.empty()) {
					parts.push_back(segment);  // Move into the new directory
				}
			}
			
			// Rebuild the new virtual directory path
			std::string newVirtualPath = "/";
			for (const auto& p : parts) {
				newVirtualPath += p + "/";
			}
			
			// If the new path is empty, we are at the root or home
			if (newVirtualPath.empty()) newVirtualPath = "/";
			
			// Resolve the real path (combine virtual path with rootDir)
			std::string realPath = resolvePathFilesystem(newVirtualPath);
			
			// Check if the real directory exists
			if (directoryExists(realPath)) {
				cmdCurrentDirectory = newVirtualPath;
			
				// Replace the first segment with "~" if it's home directory
				if (newVirtualPath.substr(0, cmdHomeDir.size()) == cmdHomeDir) {
					cmdCurrentDisplayDirectory = "~/" + newVirtualPath.substr(cmdHomeDir.size());
				} else {
					cmdCurrentDisplayDirectory = newVirtualPath;
				}
			} else {
				cmdEcho("directory not found: " + resolveDirPath(dir, 1), outputFile, append);
			}
		    cmdUserPrompt = "^]4[{" + cmdName + "}@" + "^]6[{" + cmdHostname + "} ^]4[{" + cmdCurrentDisplayDirectory + "} $ ";
		}
		void cmdClear() {
			emptyFile(cmdWindowFile.c_str());
		}
		void cmdLs(int argc, std::vector<char*> argv, std::string outputFile = cmdWindowFile, bool append = true) {
		    bool showHidden = false;
		    bool longFormat = false;
		    std::string targetDir = cmdCurrentDirectory;
		
		    // Parse flags and possible directory
		    for (int i = 1; i < argc; ++i) {
		        std::string arg = argv[i];
		        if (!arg.empty() && arg[0] == '-') {
		            for (char c : arg.substr(1)) {
		                if (c == 'a') showHidden = true;
		                if (c == 'l') longFormat = true;
		            }
		        } else {
		            targetDir = arg;
		        }
		    }
		
		    // Strip tilde if present
		    if (!targetDir.empty() && targetDir.front() == '~') {
		        targetDir = targetDir.substr(1);
		    }
		
		    // Normalize virtual path
		    std::vector<std::string> parts;
		    std::stringstream ss(targetDir);
		    std::string segment;
		    while (std::getline(ss, segment, '/')) {
		        if (!segment.empty()) parts.push_back(segment);
		    }
		
		    std::string realPath = resolvePathFilesystem(targetDir);
		    int pathType = checkPathType(realPath);
		    if (pathType == -1) {
		        cmdEcho("Directory not found: " + resolveDirPath(targetDir, 1), outputFile, append);
		        return;
		    } else if (pathType == 0) {
		        cmdEcho("Not a directory: " + resolveFilePath(targetDir, 1), outputFile, append);
		        return;
		    }
		
		    DIR* d = opendir(realPath.c_str());
		    if (!d) {
		        cmdEcho("Failed to open directory: " + resolveDirPath(targetDir, 1), outputFile, append);
		        return;
		    }
		
		    std::vector<std::string> entries;
		    struct dirent* entry;
		    while ((entry = readdir(d)) != nullptr) {
		        std::string name = entry->d_name;
		        if (!showHidden && name[0] == '.') continue;
		        entries.push_back(name);
		    }
		    closedir(d);
		
		    std::sort(entries.begin(), entries.end());
		    std::stringstream output;
		
		    for (const auto& name : entries) {
		        if (longFormat) {
		            struct stat st;
		            std::string fullPath = realPath + "/" + name;
		            if (stat(fullPath.c_str(), &st) == 0) {
		                output << ((S_ISDIR(st.st_mode)) ? "d" : "-") << " " << name << "\n";
		            } else {
		                output << "? " << name << "\n";
		            }
		        } else {
		            output << name << "  ";
		        }
		    }
		
		    //if (!longFormat) output << "\n";
		    cmdEcho(output.str(), outputFile, append);
		}
		void cmdRmdir(std::string dir, std::string outputFile, bool append) {
		    std::string realPath = resolvePathFilesystem(dir);
		
		    int pathType = checkPathType(realPath);
		    if (pathType == -1) {
		        cmdEcho("Directory does not exist: " + resolveDirPath(dir, 1), outputFile, append);
		        return;
		    }
		    if (pathType == 0) {
		        cmdEcho("Not a directory: " + resolveFilePath(dir, 1), outputFile, append);
		        return;
		    }
		
		    int status = rmdir(realPath.c_str());
		    if (status == 0) {
		        cmdEcho("Directory removed: " + resolveDirPath(dir, 1), outputFile, append);
		    } else {
		        cmdEcho("Failed to remove directory: " + resolveDirPath(dir, 1) + " (errno: " + std::to_string(errno) + ")", outputFile, append);
		    }
		}
		void cmdRm(int argc, std::vector<char*> argv, std::string outputFile = cmdWindowFile, bool append = true) {
		    bool recursive = false;
		    bool force = false;
		    std::vector<std::string> targets;
		
		    for (int i = 1; i < argc; ++i) {
		        std::string arg = argv[i];
		        if (arg.size() > 1 && arg[0] == '-') {
		            for (char c : arg.substr(1)) {
		                if (c == 'r') recursive = true;
		                else if (c == 'f') force = true;
		            }
		        } else {
		            targets.push_back(arg);
		        }
		    }
		
		    if (targets.empty()) {
		        if (!force)
		            cmdEcho("rm: missing operand", outputFile, append);
		        return;
		    }
		
		    for (const std::string& t : targets) {
				if (t == "/") { // Check for rm -rf /, more of an easter egg than anything else
					cmdEcho("Nice try", outputFile, append);
					return;
				}
		        std::string path = resolvePathFilesystem(t);
		        int type = checkPathType(path);
		
		        if (type == -1) {
		            if (!force)
		                cmdEcho("rm: cannot remove '" + t + "': No such file or directory", outputFile, append);
		            continue;
		        }
		
		        if (type == 1) { // Directory
		            if (!recursive) {
		                cmdEcho("rm: cannot remove '" + t + "': Is a directory", outputFile, append);
		                continue;
		            }
		
		            // Recursively delete
		            removeRecursive(path);
		            rmdir(path.c_str());
		        } else if (type == 0) { // Regular file
		            if (remove(path.c_str()) != 0 && !force) {
		                cmdEcho("rm: cannot remove '" + t + "'", outputFile, append);
		            }
		        }
		    }
		}
		void cmdTouch(int argc, std::vector<char*> argv, std::string outputFile = cmdWindowFile, bool append = true) {
		    bool changeAccessTimeOnly = false;
		    bool noCreate = false;
		    std::vector<std::string> files;
		
		    for (int i = 1; i < argc; ++i) {
		        std::string arg = argv[i];
		        if (arg[0] == '-' && arg.size() > 1) {
		            for (char c : arg.substr(1)) {
		                if (c == 'a') changeAccessTimeOnly = true;
		                else if (c == 'c') noCreate = true;
		            }
		        } else {
		            files.push_back(arg);
		        }
		    }
		
		    if (files.empty()) {
		        cmdEcho("touch: missing file operand", outputFile, append);
		        return;
		    }
		
		    for (const auto& file : files) {
		        std::string realPath = resolvePathFilesystem(file);
		        int pathType = checkPathType(realPath);
		
		        if (pathType == -1) {
		            if (noCreate) {
		                continue;
		            }
		
		            // Create empty file
		            std::ofstream ofs(realPath);
		            if (!ofs) {
		                cmdEcho("touch: cannot create file '" + file + "'", outputFile, append);
		                continue;
		            }
		            ofs.close();
		        }
		
		        struct stat st;
		        struct utimbuf new_times;
		
		        if (stat(realPath.c_str(), &st) == 0) {
		            time_t currentTime = time(nullptr);
		            new_times.actime = currentTime;
		            new_times.modtime = changeAccessTimeOnly ? st.st_mtime : currentTime;
		            utime(realPath.c_str(), &new_times);
		        } else {
		            cmdEcho("touch: cannot stat '" + file + "'", outputFile, append);
		        }
		    }
		}

		/*
		 * Debug commands, do not put these in help as they are not intended to really be used
		 */
		
		void cmdSidebarWidth(int newSize, std::string outputFile = cmdWindowFile, bool append = true) {
			if (newSize <= 5) { cmdEcho("Sidebar width cannot be less than 6", outputFile, append); return; };
			if (newSize > maxTermX / 2) { cmdEcho("New sidebar thiccness is too thicc, calm down", outputFile, append); return; };
			sidebarWindowWidth = (uint)newSize;
			Settings::setSetting("sidebar width", newSize);
		}
		void cmdNewPadding(int newSize, std::string outputFile = cmdWindowFile, bool append = true) {
			if (newSize >= (maxTermY / 2) - 3) { cmdEcho("Padding too padded", outputFile, append); return; };
			windowPaddingX = newSize;
			windowPaddingXDouble = windowPaddingX * 2;
			windowPaddingY = newSize;
			windowPaddingYDouble = windowPaddingY * 2;
			Settings::setSetting("window edge padding", newSize);
		}
		void cmdSaveSettings(std::string outputFile = cmdWindowFile, bool append = true) {
			Settings::saveSettings();
		}
	}
}
