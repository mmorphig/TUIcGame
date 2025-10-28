#include "settings_menu.hpp"

std::string stgSettingsFilepath;
std::string stgSelectedKey;
std::string stgSelectedGroup;

namespace Settings {    
    void setDefaultSettings() {
        settingsMap = {
            {"window edge padding", {1, SettingType::INT, 0, 10}},
            {"sidebar width", {30, SettingType::INT, 5, 120}}
        };
        settingsGroups = {
            {"Display", {"window edge padding", "sidebar width"}}
        };
        stgSelectedKey = "window edge padding";
        stgSelectedGroup = "Display";
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

    void createSettingsWindow(std::string& settingsData) {
        std::ostringstream out;

        for (const auto& groupPair : settingsGroups) {
            const std::string& groupName = groupPair.first;
            const auto& keys = groupPair.second;

            out << "[" << groupName << "]\n";

            for (const auto& key : keys) {
                auto it = settingsMap.find(key);
                if (it == settingsMap.end()) {
                    continue; // skip if key missing
                }
                fileLog("%s", const_cast<char*>(stgSelectedKey.c_str()));

                const setting& s = it->second;
                if (key == stgSelectedKey) {
                    out << "^]1b[{" << key << "}\n";
                } else {
                    out << key << "\n";
                }

                if (s.type == FLOAT || s.type == INT || s.type == UINT) {
                    double value = 0.0;
                    if (s.type == FLOAT) value = std::get<float>(s.value);
                    else if (s.type == INT) value = std::get<int>(s.value);
                    else if (s.type == UINT) value = std::get<uint>(s.value);

                    // Compute normalized position
                    double min = s.minimum;
                    double max = s.maximum;
                    if (max <= min) {
                        max = min + 1.0; // prevent divide by zero
                    }
                    double ratio = (value - min) / (max - min);
                    if (ratio < 0.0) ratio = 0.0;
                    if (ratio > 1.0) ratio = 1.0;

                    // Build bar
                    constexpr int barWidth = 20;
                    int pos = static_cast<int>(ratio * barWidth);

                    out << std::fixed << std::setprecision(2);
                    out << min << " <";
                    for (int i = 0; i < barWidth; i++) {
                        if (i == pos) out << "#";
                        else out << "-";
                    }
                    out << "> " << max << "\n";
                } 
                else {
                    // For non-numeric types just print the value
                    if (s.type == BOOL) {
                        out << (std::get<bool>(s.value) ? "true" : "false") << "\n";
                    } else if (s.type == STRING) {
                        out << "\"" << std::get<std::string>(s.value) << "\"\n";
                    }
                }
            }

            out << "\n";
        }

        settingsData = out.str();
    }
}
