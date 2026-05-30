#pragma once

#include <string>

struct AppConfig {
    std::string name = "MiSTer";
    std::string host;
    std::string username = "root";
    std::string password = "1";
};

class ConfigStore {
public:
    static AppConfig load();
    static bool save(const AppConfig& config, std::string& error);
};
