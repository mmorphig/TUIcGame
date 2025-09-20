#ifndef CXX_UTILS_HPP
#define CXX_UTILS_HPP

#include <string>
#include <chrono>
#include <ctime>

namespace Timekeeping {
    std::string getCurrentTimeHMS(); // [HH:mm:ss]
    std::string getCurrentTimeHM(); // [HH:mm]
}

#endif