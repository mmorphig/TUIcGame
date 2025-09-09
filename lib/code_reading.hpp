#ifndef CODE_READING_HPP
#define CODE_READING_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <string>
#include <unordered_map>
#include <memory>
#include <libtcc.h>
#include <vector>
#include <map>
#include <thread>
#include <cstdio>
#include <iterator>
#include <unistd.h>
#include "txt_writer.h"
#include "cmd_logic.hpp"

namespace TokenCounting {
	std::string stripStringLiterals(const std::string& line);
	std::string removeAllComments(const std::string& filename);
	int calculateTokenValue(const std::string& filename, const std::unordered_map<std::string, int>& weights, 
	                        std::unordered_map<std::string, int> tokenCounts);
};

namespace InterpretCode {	
	using FunctionPtr = void*;

    struct FunctionEntry {
	    std::string name;
	    void* pointer;
	    std::string return_type;
	};

    inline std::vector<FunctionEntry>& functionRegistry() {
        static std::vector<FunctionEntry> registry;
        return registry;
    }

    struct Registrar {
	    Registrar(const std::string& name, void* ptr, const std::string& return_type) {
	        functionRegistry().push_back({name, ptr, return_type});
	    }
	};
    
    namespace Functions {
		extern "C" inline int number() {
            return 42;
        }
		// Register 
        static Registrar reg_gainPoint("number", (void*)(intptr_t)&number, "int");
        
        // Make sure to have the static Registrar when adding more functions
	}
	
	std::string readSourceFile(const std::string& filename);
    
	int isSafe(const std::string& code);
	void run(const std::string& user_code, std::string homeDir, std::string outputFile, bool append);
	static void runCode(const std::string& user_code, const std::string& stdOutFilename, const std::string& argsString, std::string homeDir, bool redirectStdout);
	void stripAnsiCodes(const std::string& filename);
};

#endif
