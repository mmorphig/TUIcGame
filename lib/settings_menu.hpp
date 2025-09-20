#ifndef SETTINGS_MENU_HPP
#define SETTINGS_MENU_HPP

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace Settings {
    typedef enum {
        BOOL,
        FLOAT,
        INT,
        STRING
    } SettingType;

    struct setting {
        void* value;
        SettingType type;
        float minimum, maximum;
    };
    
    namespace {
        std::unordered_map<std::string, setting> settingsMap;
    }
    
    void setDefaultSettings();
}

#endif