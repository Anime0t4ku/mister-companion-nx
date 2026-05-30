#include "config.hpp"

#include <cstdio>
#include <cstring>
#include <sys/stat.h>

static constexpr const char* kConfigDir = "sdmc:/switch/mister-companion-nx";
static constexpr const char* kConfigPath = "sdmc:/switch/mister-companion-nx/config.ini";

static std::string trim(const std::string& value) {
    size_t start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

AppConfig ConfigStore::load() {
    AppConfig config;
    FILE* file = fopen(kConfigPath, "r");
    if (!file) return config;

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        std::string text = trim(line);
        if (text.empty()) continue;

        size_t separator = text.find('=');
        if (separator == std::string::npos) continue;

        std::string key = trim(text.substr(0, separator));
        std::string value = trim(text.substr(separator + 1));

        if (key == "name") config.name = value;
        else if (key == "host") config.host = value;
        else if (key == "username") config.username = value.empty() ? "root" : value;
        else if (key == "password") config.password = value.empty() ? "1" : value;
    }

    fclose(file);
    return config;
}

bool ConfigStore::save(const AppConfig& config, std::string& error) {
    mkdir("sdmc:/switch", 0777);
    mkdir(kConfigDir, 0777);

    FILE* file = fopen(kConfigPath, "w");
    if (!file) {
        error = "Unable to open config file for writing.";
        return false;
    }

    fprintf(file, "name=%s\n", config.name.c_str());
    fprintf(file, "host=%s\n", config.host.c_str());
    fprintf(file, "username=%s\n", config.username.c_str());
    fprintf(file, "password=%s\n", config.password.c_str());
    fclose(file);
    return true;
}
