#include "cxx_utils.hpp"

namespace Timekeeping {
    std::string getCurrentTimeHMS() { // [HH:mm:ss]
        time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        tm utc_time = *gmtime(&tt);
        std::string timeString = "[" + 
            ((utc_time.tm_hour < 10) ? "0" + std::to_string(utc_time.tm_hour) : std::to_string(utc_time.tm_hour)) + ":" + 
            ((utc_time.tm_min < 10) ? "0" + std::to_string(utc_time.tm_min) : std::to_string(utc_time.tm_min)) + ":" + 
            ((utc_time.tm_sec < 10) ? "0" + std::to_string(utc_time.tm_sec) : std::to_string(utc_time.tm_sec)) + "]";
        return timeString;
    }
    std::string getCurrentTimeHM() { // [HH:mm]
        time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        tm utc_time = *gmtime(&tt);
        std::string timeString = "[" + 
            ((utc_time.tm_hour < 10) ? "0" + std::to_string(utc_time.tm_hour) : std::to_string(utc_time.tm_hour)) + ":" + 
            ((utc_time.tm_min < 10) ? "0" + std::to_string(utc_time.tm_min) : std::to_string(utc_time.tm_min)) + "]";
        return timeString;
    }
}
