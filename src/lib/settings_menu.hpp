#ifndef SETTINGS_MENU_HPP
#define SETTINGS_MENU_HPP

#include <string>
#include <unordered_map>
#include <map>
#include <variant>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include "txt_writer.h"
#include "../global_version_variables.hpp"

extern std::string stgSettingsFilepath;
extern std::string stgSelectedKey;
extern std::string stgSelectedGroup;

namespace Settings {
    typedef enum {
        BOOL,
        FLOAT,
        INT,
        UINT,
        STRING
    } SettingType;

    struct setting {
        std::variant<bool, float, int, uint, std::string> value;
        SettingType type;
        float minimum, maximum;
    };

    namespace {
        std::unordered_map<std::string, setting> settingsMap;
        std::map<std::string, std::vector<std::string>> settingsGroups;
    }

    template <typename T>
    inline bool setSetting(std::string key, T value) {
        auto& setting = settingsMap.at(key);

        switch (setting.type) {
            case (SettingType::UINT): 
	            value = std::abs(value); // Set value to the absolute, then keep going into the int setting
            case (SettingType::INT): {
                if (sizeof(value) != sizeof(int)) { return false; }
                float fValue = static_cast<float>(value);
                if (fValue < setting.minimum) fValue = setting.minimum;
                if (fValue > setting.maximum) fValue = setting.maximum;
                setting.value = static_cast<T>(fValue);
                break;
            }
            default: { // bool or string
                setting.value = value;
                break;
            }
        }
        return true;
    }
    
    void setDefaultSettings();
    void saveSettings();
    void createSettingsWindow(std::string& settingsData);
}

#endif