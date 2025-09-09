#include "game_elements.hpp"

int maxTokens = 100; // Initial max tokens the user can have in their inputted code

std::unordered_map<std::string, int> weights = {
	{"namespace", 3},
	{"class", 4},
	{"function_definition", 5},
	{"function_call", 2},
	{"variable", 1},
	{"loop", 3},
	{"operator", 1},
	{"conditional", 2},
	{"string", 1},
	{"enum", 2},
	{"struct", 3}
};

std::unordered_map<std::string, int> tokenCounts = {
	{"namespace", 0},
	{"class", 0},
	{"function_definition", 0},
	{"function_call", 0},
	{"variable", 0},
	{"loop", 0},
	{"operator", 0},
	{"conditional", 0},
	{"string", 0},
	{"enum", 0},
	{"struct", 0}
};

// Unlocked types of code, defaults with functions, operators and conditionals (if/else and switch)
std::unordered_map<std::string, int> unlockedBTypes = { 
	{"namespace", 0},
	{"class", 0},
	{"function", 1},
	{"variable", 0},
	{"loop", 0},
	{"operator", 1},
	{"conditional", 0},
	{"string", 0},
	{"enum", 0},
	{"struct", 0}
};
