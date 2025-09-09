#include "code_reading.hpp"

namespace TokenCounting {
	std::string stripStringLiterals(const std::string& line) {
	    std::string result;
	    bool inString = false;
	    char quoteType = 0;
	
	    for (size_t i = 0; i < line.size(); ++i) {
	        char c = line[i];
	        if (!inString) {
	            if (c == '"' || c == '\'') {
	                inString = true;
	                quoteType = c;
	                result += ' '; 
	            } else {
	                result += c;
	            }
	        } else {
	            if (c == quoteType && line[i - 1] != '\\') {
	                inString = false;
	                result += ' ';
	            } else {
	                result += ' ';
	            }
	        }
	    }
	
	    return result;
	}
	
	std::string removeAllComments(const std::string& filename) {
	    std::ifstream file(filename);
	    if (!file.is_open()) {
	        std::cerr << "Error opening file: " << filename << "\n";
	        return "";
	    }
	
	    std::string result;
	    std::string line;
	    bool inMultiLineComment = false;
	
	    while (std::getline(file, line)) {
	        size_t i = 0;
	
	        while (i < line.size()) {
	            if (inMultiLineComment) {
	                if (i + 1 < line.size() && line[i] == '*' && line[i + 1] == '/') {
	                    inMultiLineComment = false;
	                    i += 2; 
	                } else {
	                    ++i;
	                }
	            }
	            else if (i + 1 < line.size() && line[i] == '/' && line[i + 1] == '*') {
	                inMultiLineComment = true;
	                i += 2; 
	            }
	            else if (i + 1 < line.size() && line[i] == '/' && line[i + 1] == '/') {
	                break; 
	            }
	            else {
	                result += line[i];
	                ++i;
	            }
	        }
	
	        result += '\n';
	    }
	
	    file.close();
	    return result;
	}
	
	int calculateTokenValue(const std::string& filename,
	                        const std::unordered_map<std::string, int>& weights,
	                        std::unordered_map<std::string, int>& tokenCounts) {
	    std::string cleanFileContent = removeAllComments(filename);
	    if (cleanFileContent.empty()) {
	        return -1; 
	    }
	
	    std::istringstream cleanFileStream(cleanFileContent);
	    std::string line;
	    int totalScore = 0;
	
		// Thank you ChatGPT for most of the Regex
	    std::regex namespaceRegex(R"(\bnamespace\s+[a-zA-Z_][a-zA-Z0-9_]*\s*\{)");
	    std::regex classRegex(R"(\bclass\s+[a-zA-Z_][a-zA-Z0-9_]*\s*(\:\s*public\s+[a-zA-Z_][a-zA-Z0-9_]*)?\s*\{)");
	    std::regex functionDefRegex(R"((\b[a-zA-Z_][a-zA-Z0-9_<>:]*)\s+(\**[a-zA-Z_][a-zA-Z0-9_]*)\s*\([^)]*\)\s*\{)");
	    std::regex functionCallRegex(R"([a-zA-Z_][a-zA-Z0-9_]*\s*\([^;{}]*\)\s*;)");
	    std::regex variableRegex(R"((int|float|double|char|bool|string)\s+\**[a-zA-Z_][a-zA-Z0-9_]*(\s*=\s*[^;]+)?\s*;|#define\s+[a-zA-Z_][a-zA-Z0-9_]*\s+.+)");
	    std::regex loopRegex(R"(\b(for|while|do)\b)");
	    std::regex operatorRegex(R"([\+\-\*/%=!<>&\|]{1,2})");
	    std::regex ifRegex(R"(\bif\s*\()");
	    std::regex elseRegex(R"(\belse\b)");
	    std::regex switchRegex(R"(\bswitch\s*\()");
	    std::regex caseRegex(R"(\bcase\s+.*:)");
	    std::regex stringRegex(R"(["'](\\.|[^\\])*?["'])");
	    std::regex enumRegex(R"(\benum\b.*?\{)");
		std::regex structRegex(R"(\bstruct\b.*?\{)");
	
	    while (std::getline(cleanFileStream, line)) {
		    if (std::regex_match(line, std::regex(R"(^\s*$)"))) continue;
		
			std::regex stringLiteralRegex(R"(["'](\\.|[^\\])*?["'])");
		    std::smatch match;
		    auto tempLine = line;
		    while (std::regex_search(tempLine, match, stringRegex)) {
		        tokenCounts["string"]++;
		        totalScore += weights.at("string");
		        tempLine = match.suffix();
		    }
	    
		    std::string strippedLine = stripStringLiterals(line);
		
	        if (std::regex_search(strippedLine, namespaceRegex)) {
	            tokenCounts["namespace"]++;
	            totalScore += weights.at("namespace");
	        }
	
	        if (std::regex_search(strippedLine, classRegex)) {
	            tokenCounts["class"]++;
	            totalScore += weights.at("class");
	        }
	
	        if (std::regex_search(strippedLine, functionDefRegex)) {
	            tokenCounts["function_definition"]++;
	            totalScore += weights.at("function_definition");
	        }
	
	        if (std::regex_search(strippedLine, functionCallRegex) &&
	            !std::regex_search(strippedLine, functionDefRegex)) {
	            tokenCounts["function_call"]++;
	            totalScore += weights.at("function_call");
	        }
	
	        auto line_copy = strippedLine;
	
	        while (std::regex_search(line_copy, match, variableRegex)) {
	            tokenCounts["variable"]++;
	            totalScore += weights.at("variable");
	            line_copy = match.suffix();
	        }
	
	        if (std::regex_search(strippedLine, loopRegex)) {
	            tokenCounts["loop"]++;
	            totalScore += weights.at("loop");
	        }
	        
	        if (std::regex_search(strippedLine, enumRegex)) {
	            tokenCounts["enum"]++;
	            totalScore += weights.at("enum");
	        }
	        if (std::regex_search(strippedLine, structRegex)) {
	            tokenCounts["struct"]++;
	            totalScore += weights.at("struct");
	        }
	
	        line_copy = strippedLine;
	        while (std::regex_search(line_copy, match, operatorRegex)) {
	            tokenCounts["operator"]++;
	            totalScore += weights.at("operator");
	            line_copy = match.suffix();
	        }
	
	        if (std::regex_search(strippedLine, ifRegex) ||
	            std::regex_search(strippedLine, elseRegex) ||
	            std::regex_search(strippedLine, switchRegex) ||
	            std::regex_search(strippedLine, caseRegex)) {
	            tokenCounts["conditional"]++;
	            totalScore += weights.at("conditional");
	        }
	    }
	
	    return totalScore;
	}
}

// "InterpretCode" is technically a misnomer, it does compile with tcc
namespace InterpretCode {
    // Load C code from a file
    std::string readSourceFile(const std::string& filename) {
        std::ifstream in(filename);
        if (!in) throw std::runtime_error("Cannot open source file.");
        std::stringstream buffer;
        buffer << in.rdbuf();
        return buffer.str();
    }

	int isSafe(const std::string& code) {
	    // Keywords to reject outright, mostly a quick little check before bringing out the big guns
	    static const std::vector<std::string> blacklist = {
	        /*"system(",*/ "exec", "fork", "volatile", "asm", "__asm"
	    };
	    for (const auto& keyword : blacklist) {
	        if (code.find(keyword) != std::string::npos) {
	            return 0;
	        }
	    }
	    
		// The big guns
	    static const std::vector<std::regex> dangerous_patterns = {
	        // Infinite loops
	        std::regex(R"(while\s*\(\s*(1|true)\s*\))", std::regex::icase),
	        std::regex(R"(for\s*\(\s*;;\s*\))"),
	
	        // Dangerous functions
	        std::regex(R"(\bsystem\s*\()", std::regex::icase),
	        std::regex(R"(\bfork\s*\()", std::regex::icase),
	
	        // Inline assembly
	        std::regex(R"(\b__?asm__?\b)", std::regex::icase),
	
	        // Any #include that isn't stdio.h 
	        //std::regex(R"(#\s*include\s*<(?!(stdio\.h))[^>]*>)", std::regex::icase)
	    };
	
	    for (const auto& pattern : dangerous_patterns) {
	        if (std::regex_search(code, pattern)) {
	            return 0;
	        }
	    }
	    // Test for interacive input, this is unsupported, so it is rejected
	    static const std::vector<std::string> inblacklist = {
	        "stdin", "print", "stdout"
	    };
	    for (const auto& keyword : inblacklist) {
	        if (code.find(keyword) != std::string::npos) {
	            return 1;
	        }
	    }
	
	    return 2;
	}
    
    void run(const std::string& codeFile, std::string homeDir, std::string outputFile, bool append) {
		std::string user_code = CmdStuff::readFile(codeFile);
		if (!isSafe(user_code)) {
		    CmdStuff::Running::writeOutput("Code rejected: unsafe content detected.\n", outputFile, append);
		    return;
		} else if (isSafe(user_code) == 1) {
			CmdStuff::Running::writeOutput("Code error: interactive IO is unsupported. (stdout & stdin)\n", outputFile, append);
		    return;
		}
		std::thread sandboxThread([&]() {
		    try {
		        runCode(user_code, outputFile, "-u mmorphig -i 127.0.0.1 -k ~/.ssh/id_rsa_no_pass -h /home/mmorphig/mc-server -s", homeDir, true);
		    } catch (...) {
		        CmdStuff::Running::writeOutput("Exception caught in code.\n", outputFile, true);
		    }
		});
		sandboxThread.join();
	};

    // Interpreter core function, must be static
	static void runCode(const std::string& user_code, const std::string& stdOutFilename, const std::string& argsString, std::string homeDir, bool append) {
		//std::cout << "[Interpreter] Starting interpretation.\n";
		
	    TCCState* state = tcc_new();
	    if (!state) {
	        //std::cerr << "[Interpreter] Failed to create TCC state." << std::endl;
	        return;
	    }
	
	    tcc_set_output_type(state, TCC_OUTPUT_MEMORY);
	
	    for (const auto& entry : functionRegistry()) {
	        tcc_add_symbol(state, entry.name.c_str(), entry.pointer);
	    }
	
	    std::stringstream header;
	    header << "#include <stdio.h>\n";
	    
	    std::string includePath = homeDir;
	    tcc_add_include_path(state, includePath.c_str());
	    
		tcc_add_library(state, "m");
	
	    for (const auto& entry : functionRegistry()) {
	        header << "extern " << entry.return_type << " " << entry.name << "();\n";
	    }
	
	    std::string full_code = header.str() + "\n" + user_code;
	
	    if (tcc_compile_string(state, full_code.c_str()) == -1) {
	        //std::cerr << "[Interpreter] Compilation failed." << std::endl;
	        tcc_delete(state);
	        return;
	    }
	
	    if (tcc_relocate(state) < 0) {
	        //std::cerr << "[Interpreter] Relocation failed." << std::endl;
	        tcc_delete(state);
	        return;
	    }
		
		// FIND A THREAD-SAFE WAY OF DOING THIS
	    /*FILE* file = freopen(stdOutFilename.c_str(), "w", stdout);
	    if (!file) {
	        //std::cerr << "[Interpreter] Failed to redirect stdout to file." << std::endl;
	        tcc_delete(state);
	        return;
	    }*/
	
	    // Parse argsString into argv-style vector
	    std::istringstream iss(argsString);
	    std::vector<std::string> args{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>()};
	
	    // Add dummy name as argv[0]
	    args.insert(args.begin(), "mega-hack_69420.x86_64");
	
	    int argc = static_cast<int>(args.size());
	    std::vector<char*> argv;
	    for (auto& arg : args) {
	        argv.push_back(&arg[0]); // get mutable char*
	    }
	    argv.push_back(nullptr); // null-terminate
	
	    using MainFunc = int (*)(int, char**);
	    MainFunc func = (MainFunc)tcc_get_symbol(state, "main");
	    if (!func) {
	        //std::cerr << "[Interpreter] main(int, char**) not found." << std::endl;
	        tcc_delete(state);
	        return;
	    }
	    
		sleep(1);
	    int result = func(argc, argv.data());
	
	    fflush(stdout);
	    //if (file) {fclose(file);}
	
	    //std::cout << "[Interpreter] main() returned: " << result << std::endl;
	
	    tcc_delete(state);
	
	    //stripAnsiCodes(stdOutFilename);
	}

	void stripAnsiCodes(const std::string& filename) {
	    std::ifstream in(filename);
	    std::ostringstream contents;
	    contents << in.rdbuf();
	    std::string text = contents.str();
	
	    // Regex to remove ANSI escape codes
	    std::regex ansiRegex(R"(\x1B\[[0-9;?]*[a-zA-Z])");
	    text = std::regex_replace(text, ansiRegex, "");
	
	    std::ofstream out(filename);
	    out << text;
	}
	
}
