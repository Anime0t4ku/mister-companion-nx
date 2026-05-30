#pragma once

#include <switch.h>

#include "config.hpp"
#include "ssh_client.hpp"

#include <string>
#include <vector>

class App {
public:
    void run();

private:
    enum class Tab {
        Connection,
        Device,
    };

    AppConfig config;
    SshClient ssh;
    Tab tab = Tab::Connection;
    int selected = 0;
    std::string status = "Disconnected";
    std::string lastMessage;
    std::string sdStorage = "Not refreshed";
    std::string usbStorage = "Not refreshed";
    std::string smbStatus = "Not refreshed";
    std::string nowPlaying;

    void draw();
    void drawConnection();
    void drawDevice();
    void drawFooter();

    void handleInput(u64 buttons);
    void handleConnectionInput(u64 buttons);
    void handleDeviceInput(u64 buttons);

    void editText(const char* title, std::string& value, bool password = false);
    bool confirm(const char* title, const char* body);

    void saveConfig();
    void connectOrDisconnect();

    void refreshDevice();
    void refreshStorage();
    void refreshSmb();
    void refreshNowPlaying();

    void toggleSmb();
    void reboot();
    void shutdown();
    void returnToMenu();

    std::string runCommandMessage(const std::string& command);
    std::string formatDfLine(const std::string& line);
    std::string prettifyGameName(const std::string& path);
    std::string normalizeCoreName(std::string core);
};