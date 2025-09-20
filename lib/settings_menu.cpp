#include "settings_menu.hpp"

std::string stgSettingsFilepath;

namespace Settings {    
    void setDefaultSettings() {
        settingsMap = {
            {"window edge padding", {1, SettingType::INT, 0, 10}},
            {"sidebar width", {30, SettingType::INT, 5, 120}}
        };
    }

    void saveSettings() {
        nlohmann::json j;
        j["_version"] = SETTINGS_VERSION;

        for (const auto& [key, setting] : settingsMap) {
            std::visit([&](auto&& arg) {
                j[key] = arg;
            }, setting.value);
        }

        std::ofstream out(stgSettingsFilepath);
        if (!out) {
            throw std::runtime_error("Failed to open " + stgSettingsFilepath);
        }
        out << j.dump(4);
    }
}
