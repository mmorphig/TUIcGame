#ifndef GAME_ELEMENTS_CPP
#define GAME_ELEMENTS_CPP

#include <unordered_map>
#include <string>
#include <vector>

extern int maxTokens; // Initial max tokens the user can have in their inputted code

extern std::unordered_map<std::string, int> weights;

extern std::unordered_map<std::string, int> tokenCounts;

// Unlocked types of code, defaults with functions, operators and conditionals (if/else and switch)
extern std::unordered_map<std::string, int> unlockedBTypes;

#endif
