#pragma once

#include <switch.h>

#include "config.hpp"
#include "remote_client.hpp"
#include "ssh_client.hpp"
#include "ui.hpp"

#include <string>
#include <vector>

class App {
public:
    void run();

private:
    enum class Tab {
        Connection,
        Device,
        Remote,
    };

    AppConfig config;
    SshClient ssh;
    RemoteClient remote;
    UiRenderer ui;
    Tab tab = Tab::Connection;
    int selected = 0;
    std::string status = "Disconnected";
    std::string lastMessage;
    std::string sdStorage = "Not refreshed";
    std::string usbStorage = "Not refreshed";
    std::string smbStatus = "Not refreshed";
    std::string nowPlaying;
    std::string remoteInstalled = "Not checked";
    std::string remoteRunning = "Not checked";
    std::string remoteStartup = "Not checked";
    bool passthroughActive = false;

    void draw();
    void drawHeader();
    void drawConnection();
    void drawDevice();
    void drawRemote();
    void drawPassthrough();

    void handleInput(u64 buttons);
    void handleConnectionInput(u64 buttons);
    void handleDeviceInput(u64 buttons);
    void handleRemoteInput(u64 buttons);
    void handlePassthroughInput(u64 down, u64 up, u64 held);

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
    void returnToMenu();

    void refreshRemoteStatus();
    void installRemoteDaemon();
    void startRemoteDaemon();
    void stopRemoteDaemon();
    void toggleRemoteStartup();
    void uninstallRemoteDaemon();
    void startPassthrough();
    void stopPassthrough();
    void sendPassthroughButton(u64 mask, u64 buttons, const std::string& control, const std::string& name, const std::string& action);

    std::string runCommandMessage(const std::string& command);
    std::string formatDfLine(const std::string& line);
    std::string prettifyGameName(const std::string& path);
    std::string normalizeCoreName(std::string core);
};
