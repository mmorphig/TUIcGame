#ifndef CMD_LOGIC_HPP
#define CMD_LOGIC_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <filesystem>
#include <openssl/evp.h>
#include <iomanip>
#include <ctime>
#include <utime.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <algorithm>
#include "txt_writer.h" // This file uses some functions from here, might as well re-use functions :)
#include "txt_reader.h"
#include "code_reading.hpp"
#include "menu_logic.hpp"

/* RULES
 * 
 * Directories must end with a /
 * Everything that starts with cmd is for something in-game
 */

extern std::string rootDir; // root and home dir in-game, these are from the actual filesystem, pointing from ./save
extern std::string homeDir;
extern std::string cmdName;
extern std::string cmdHomeDir; // Directory for the in-game users home, used to remove a bunch of "/home/" + name
extern std::string cmdText; // Text the user has inputted into the cmd
extern std::string cmdCurrentDirectory; // current working directory of the cmd in-game
extern std::string cmdCurrentDisplayDirectory; // current working directory of the cmd but to show the user
extern std::string cmdUserPrompt; // "name@hostname cmdCurrentDirectory $ "
extern std::string cmdHostname; // in-game system hostname
extern std::string cmdWindowFile; // File that holds the data for the main window, which acts as the stdout in-game
extern std::string cmdHistoryFile; // bash history in-game
extern int cmdHistoryOffset; // Offset for the currento position in the history file
extern int cmdHistoryFileLength;
extern std::string cmdTextCache; // Holds the current cmdText in cache while cmdText is what the user sees from the history file

namespace CmdStuff {
	std::string inline readFile(const std::string& filename) {
        std::ifstream in(filename);
        if (!in) {
			std::string cmdError = "Cannot open file: " + filename;
			writeLineToFile(cmdWindowFile.c_str(), cmdError.c_str());
		}
        std::stringstream buffer;
        buffer << in.rdbuf();
        return buffer.str();
    }
    bool inline directoryExists(const std::string& path) {
	    return std::filesystem::is_directory(path);
	}
	int inline checkPathType(const std::string& path) {
	    struct stat pathStat;
	    if (stat(path.c_str(), &pathStat) != 0) {
	        return -1; // Does not exist
	    }
	    if (S_ISDIR(pathStat.st_mode)) {
	        return 1;  // Directory
	    }
	    if (S_ISREG(pathStat.st_mode)) {
	        return 0;  // Regular file
	    }
	    return -1; // Not a file or directory (e.g., socket, device, etc.)
	}
	std::string inline resolvePathFilesystem(const std::string& input) {
	    if (input.empty()) return "";
	
	    if (input[0] == '/') {
	        return rootDir + input.substr(1);
	    } else if (input[0] == '~') {
	        return homeDir + input.substr(2);
	    } else if (input.size() >= 2 && input[0] == '.' && input[1] == '/') {
	        return rootDir + cmdCurrentDirectory.substr(1) + input.substr(2);
	    }
	
	    return rootDir + cmdCurrentDirectory.substr(1) + input;
	}
	std::string inline resolveDirPath(const std::string& input, int addCurr = 0) { // converts a path from ex: /path/to/dir to path/to/dir/
	    if (input.empty()) return "";
	
	    std::string path = input;
	
	    // Remove leading slash if it exists
	    if (path.front() == '/')
	        path = path.substr(1);
	
	    // Ensure trailing slash
	    if (path.back() != '/')
	        path += '/';
	
	    if (addCurr) { return cmdCurrentDirectory + path; } else { return path;}
	}
	std::string inline resolveFilePath(const std::string& input, int addCurr = 0) { // converts a path from ex: /path/to/file to path/to/file, simple
	    if (input.empty()) return "";
	
	    std::string path = input;
	
	    // Remove leading slash if it exists
	    if (path.front() == '/')
	        path = path.substr(1);
	
	    if (addCurr) { return cmdCurrentDirectory + path; } else { return path;}
	}
	std::string inline replaceTilde(const std::string& path) {
	    if (!path.empty() && path[0] == '~') {
	        return cmdHomeDir + path.substr(2); // remove '~', append rest
	    }
	    return path; // no change needed
	}
	void inline removeRecursive(const std::string& path) {
	    DIR* dir = opendir(path.c_str());
	    if (!dir) return;
	
	    struct dirent* entry;
	    while ((entry = readdir(dir)) != nullptr) {
	        std::string name = entry->d_name;
	        if (name == "." || name == "..") continue;
	
	        std::string fullPath = path + "/" + name;
	
	        struct stat st;
	        if (stat(fullPath.c_str(), &st) == 0) {
	            if (S_ISDIR(st.st_mode)) {
	                removeRecursive(fullPath);
	                rmdir(fullPath.c_str());
	            } else {
	                remove(fullPath.c_str());
	            }
	        }
	    }
	    closedir(dir);
	}
	
	namespace Reading { // Reading the cmd and interpreting it
		std::string inline hashFileMD5(const std::string& filename) { // Check to see if the 
		    std::ifstream file(filename, std::ios::binary);
		    if (!file) throw std::runtime_error("Cannot open file");
		
		    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
		    if (!ctx) throw std::runtime_error("Failed to create EVP_MD_CTX");
		
		    if (EVP_DigestInit_ex(ctx, EVP_md5(), nullptr) != 1)
		        throw std::runtime_error("EVP_DigestInit_ex failed");
		
		    char buffer[8192];
		    while (file.good()) {
		        file.read(buffer, sizeof(buffer));
		        if (EVP_DigestUpdate(ctx, buffer, file.gcount()) != 1)
		            throw std::runtime_error("EVP_DigestUpdate failed");
		    }
		
		    unsigned char hash[EVP_MAX_MD_SIZE];
		    unsigned int hashLen = 0;
		    if (EVP_DigestFinal_ex(ctx, hash, &hashLen) != 1)
		        throw std::runtime_error("EVP_DigestFinal_ex failed");
		
		    EVP_MD_CTX_free(ctx);
		
		    std::stringstream ss;
		    for (unsigned int i = 0; i < hashLen; ++i)
		        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
		
		    return ss.str();
		}
		std::vector<std::string> split(const std::string& str);
		void interpretCmdText(std::string cmdText);
	}
	
	namespace Running { // Actually running the commands
		void inline writeOutput(const std::string& text, const std::string& outputFile, bool append) {
		    std::ofstream out;
		    if (append) {
		        out.open(outputFile, std::ios::out | std::ios::app);
		    } else {
		        out.open(outputFile, std::ios::out | std::ios::trunc);
		    }
		
		    if (!out) {
		        // Optionally handle the error or log it
		        return;
		    }
		
		    out << text << std::endl;
		    out.close();
		}
		
		void inline cmdEcho(std::string text, std::string outputFile, bool append) { // Only inlined cmd function because it is small
			writeOutput(text.c_str(), outputFile.c_str(), append);
		}
		void cmdRun(std::string file, std::string outputFile, bool append);
		void cmdReset(std::string outputFile, bool append); // uses the output file to throw errors, is not meant to output anything
		void cmdCat(std::string file, std::string outputFile, bool append);
		void cmdCd(std::string dir, std::string outputFile, bool append);
		void cmdClear();
		void cmdLs(int argc, std::vector<char*> argv, std::string outputFile, bool append);
		void cmdMkdir(std::string dir, std::string outputFile, bool append);
		void cmdRmdir(std::string dir, std::string outputFile, bool append);
		void cmdRm(int argc, std::vector<char*> argv, std::string outputFile, bool append);
		void cmdTouch(int argc, std::vector<char*> argv, std::string outputFile, bool append);
		void cmdHelp(std::string outputFile, bool append);
	}
}
#endif
