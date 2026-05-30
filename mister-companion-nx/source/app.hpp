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
        Scripts,
    };

    enum class ScriptId {
        UpdateAll,
        Zaparoo,
        MigrateSd,
        CifsMount,
        AutoTime,
        CdGameOrganizer,
        DavBrowser,
        FtpSaveSync,
        StaticWallpaper,
        Syncthing,
        RaViewer,
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
    int selectedScript = 0;
    std::vector<std::vector<std::string>> cachedScriptStatus;

    void draw();
    void drawHeader();
    void drawConnection();
    void drawDevice();
    void drawRemote();
    void drawScripts();
    void drawPassthrough();

    void handleInput(u64 buttons);
    void handleConnectionInput(u64 buttons);
    void handleDeviceInput(u64 buttons);
    void handleRemoteInput(u64 buttons);
    void handleScriptsInput(u64 buttons);
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

    std::string scriptTitle(ScriptId id) const;
    std::vector<std::string> scriptActions(ScriptId id);
    std::vector<std::string> scriptStatus(ScriptId id);
    void refreshCurrentScriptStatus();
    void executeScriptAction(ScriptId id, int actionIndex);
    void configureUpdateAll();
    void configureCifsMount();
    void configureDavBrowser();
    void configureFtpSaveSync();
    void configureRaViewer();
    void scriptInstall(const std::string& title, const std::string& path, const std::string& url);
    void scriptUninstall(const std::string& title, const std::string& command);
    void runScriptCommand(const std::string& title, const std::string& command, bool confirmFirst = false);
    void showOutputWindow(const std::string& title, const std::string& output);
    void showStreamingCommandWindow(const std::string& title, const std::string& command, const std::string& successMessage, const std::string& failureMessage);
    bool askField(const char* title, std::string& value, bool password = false);
    bool askYesNo(const char* title, const char* body, bool defaultYes = false);
    void ensureScriptsDirs();

    std::string runCommandMessage(const std::string& command);
    std::string formatDfLine(const std::string& line);
    std::string prettifyGameName(const std::string& path);
    std::string normalizeCoreName(std::string core);
};
