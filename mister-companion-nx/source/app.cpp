#include "app.hpp"

#include <switch.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <sstream>

static std::vector<std::string> splitWords(const std::string& value) {
    std::stringstream stream(value);
    std::vector<std::string> parts;
    std::string part;
    while (stream >> part) parts.push_back(part);
    return parts;
}

static std::string trim(std::string value) {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\r' || value.front() == '\n' || value.front() == '\t')) value.erase(value.begin());
    while (!value.empty() && (value.back() == ' ' || value.back() == '\r' || value.back() == '\n' || value.back() == '\t')) value.pop_back();
    return value;
}

void App::run() {
    config = ConfigStore::load();

    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        padUpdate(&pad);
        const u64 buttons = padGetButtonsDown(&pad);

        if (buttons & HidNpadButton_Plus) break;

        handleInput(buttons);
        draw();
        consoleUpdate(NULL);
    }
}

void App::draw() {
    printf("\x1b[2J\x1b[H");
    printf("MiSTer Companion NX\n");
    printf("===================\n\n");
    printf("[%s Connection]  [%s Device]\n\n", tab == Tab::Connection ? "*" : " ", tab == Tab::Device ? "*" : " ");

    if (tab == Tab::Connection) drawConnection();
    else drawDevice();

    drawFooter();
}

void App::drawConnection() {
    const char* marker[] = {">", " ", " ", " ", " ", " ", " "};
    for (int i = 0; i < 7; i++) marker[i] = selected == i ? ">" : " ";

    printf("Connection\n");
    printf("----------\n");
    printf("%s Name:     %s\n", marker[0], config.name.c_str());
    printf("%s IP/Host:  %s\n", marker[1], config.host.empty() ? "Not set" : config.host.c_str());
    printf("%s Username: %s\n", marker[2], config.username.c_str());
    printf("%s Password: %s\n", marker[3], config.password.empty() ? "Not set" : "********");
    printf("%s Save profile\n", marker[4]);
    printf("%s %s\n", marker[5], ssh.isConnected() ? "Disconnect" : "Connect over SSH");
    printf("%s Test command: echo connected\n\n", marker[6]);
    printf("Status: %s\n", status.c_str());
    if (!lastMessage.empty()) printf("Message: %s\n", lastMessage.c_str());
}

void App::drawDevice() {
    const char* marker[] = {">", " ", " ", " ", " "};
    for (int i = 0; i < 5; i++) marker[i] = selected == i ? ">" : " ";

    printf("Device\n");
    printf("------\n");
    printf("MiSTer: %s\n", config.host.empty() ? "Not set" : config.host.c_str());
    printf("Status: %s\n\n", ssh.isConnected() ? "Connected" : "Disconnected");
    printf("SD:  %s\n", sdStorage.c_str());
    printf("USB: %s\n", usbStorage.c_str());
    printf("SMB: %s\n", smbStatus.c_str());
    if (!nowPlaying.empty()) printf("Now Playing: %s\n", nowPlaying.c_str());
    printf("\n");
    printf("%s Refresh device info\n", marker[0]);
    printf("%s Toggle SMB startup\n", marker[1]);
    printf("%s Return to MiSTer menu\n", marker[2]);
    printf("%s Reboot MiSTer\n", marker[3]);
    printf("%s Disconnect\n\n", marker[4]);
    if (!lastMessage.empty()) printf("Message: %s\n", lastMessage.c_str());
}

void App::drawFooter() {
    printf("\nControls: Up/Down Select  A Confirm/Edit  L/R Switch Tab  + Exit\n");
}

void App::handleInput(u64 buttons) {
    if (buttons & HidNpadButton_L) {
        tab = Tab::Connection;
        selected = 0;
        return;
    }
    if (buttons & HidNpadButton_R) {
        tab = Tab::Device;
        selected = 0;
        return;
    }

    if (tab == Tab::Connection) handleConnectionInput(buttons);
    else handleDeviceInput(buttons);
}

void App::handleConnectionInput(u64 buttons) {
    if (buttons & HidNpadButton_Up) selected = (selected + 6) % 7;
    if (buttons & HidNpadButton_Down) selected = (selected + 1) % 7;
    if (!(buttons & HidNpadButton_A)) return;

    switch (selected) {
        case 0: editText("Device name", config.name); break;
        case 1: editText("MiSTer IP or hostname", config.host); break;
        case 2: editText("SSH username", config.username); break;
        case 3: editText("SSH password", config.password, true); break;
        case 4: saveConfig(); break;
        case 5: connectOrDisconnect(); break;
        case 6: lastMessage = runCommandMessage("echo connected"); break;
    }
}

void App::handleDeviceInput(u64 buttons) {
    if (buttons & HidNpadButton_Up) selected = (selected + 4) % 5;
    if (buttons & HidNpadButton_Down) selected = (selected + 1) % 5;
    if (!(buttons & HidNpadButton_A)) return;

    switch (selected) {
        case 0: refreshDevice(); break;
        case 1: toggleSmb(); break;
        case 2: returnToMenu(); break;
        case 3: reboot(); break;
        case 4:
            ssh.disconnect();
            status = "Disconnected";
            lastMessage = "Disconnected.";
            break;
    }
}

void App::editText(const char* title, std::string& value, bool password) {
    SwkbdConfig keyboard;
    char output[256] = {0};

    Result rc = swkbdCreate(&keyboard, 0);
    if (R_FAILED(rc)) {
        lastMessage = "Unable to open software keyboard.";
        return;
    }

    if (password) swkbdConfigMakePresetPassword(&keyboard);
    else swkbdConfigMakePresetDefault(&keyboard);

    swkbdConfigSetHeaderText(&keyboard, title);
    swkbdConfigSetInitialText(&keyboard, value.c_str());
    swkbdConfigSetStringLenMax(&keyboard, sizeof(output) - 1);

    rc = swkbdShow(&keyboard, output, sizeof(output));
    swkbdClose(&keyboard);

    if (R_SUCCEEDED(rc)) {
        value = output;
        lastMessage = "Value updated.";
    }
}

bool App::confirm(const char* title, const char* body) {
    int choice = 0;
    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        printf("\x1b[2J\x1b[H");
        printf("%s\n", title);
        printf("================\n\n");
        printf("%s\n\n", body);
        printf("%s Yes\n", choice == 0 ? ">" : " ");
        printf("%s No\n\n", choice == 1 ? ">" : " ");
        printf("A Confirm  B Cancel\n");
        consoleUpdate(NULL);

        padUpdate(&pad);
        u64 buttons = padGetButtonsDown(&pad);
        if (buttons & HidNpadButton_Up) choice = 0;
        if (buttons & HidNpadButton_Down) choice = 1;
        if (buttons & HidNpadButton_A) return choice == 0;
        if (buttons & HidNpadButton_B) return false;
    }

    return false;
}

void App::saveConfig() {
    std::string error;
    if (ConfigStore::save(config, error)) lastMessage = "Profile saved to SD card.";
    else lastMessage = error;
}

void App::connectOrDisconnect() {
    if (ssh.isConnected()) {
        ssh.disconnect();
        status = "Disconnected";
        lastMessage = "Disconnected.";
        return;
    }

    status = "Connecting...";
    draw();
    consoleUpdate(NULL);

    std::string message;
    if (ssh.connect(config, message)) {
        status = "Connected";
        lastMessage = message;
        saveConfig();
        refreshDevice();
    } else {
        status = "Disconnected";
        lastMessage = message;
    }
}

std::string App::runCommandMessage(const std::string& command) {
    if (!ssh.isConnected()) return "No active SSH connection.";
    SshResult result = ssh.runCommand(command);
    if (result.success) return result.output.empty() ? "Command completed." : result.output;
    return result.error.empty() ? "Command failed." : result.error;
}

void App::refreshDevice() {
    if (!ssh.isConnected()) {
        lastMessage = "No active SSH connection.";
        return;
    }
    refreshStorage();
    refreshSmb();
    refreshNowPlaying();
    lastMessage = "Device info refreshed.";
}

void App::refreshStorage() {
    SshResult sd = ssh.runCommand("df -h /media/fat | tail -1");
    sdStorage = sd.success ? formatDfLine(sd.output) : "Unable to read storage";

    SshResult usb = ssh.runCommand("df -h | grep /media/usb | head -1");
    usbStorage = usb.success && !usb.output.empty() ? formatDfLine(usb.output) : "No USB storage detected";
}

void App::refreshSmb() {
    SshResult result = ssh.runCommand("test -f /media/fat/linux/samba.sh && echo enabled || echo disabled");
    if (!result.success) {
        smbStatus = "Unknown";
        return;
    }
    smbStatus = trim(result.output) == "enabled" ? "Enabled" : "Disabled";
}

void App::refreshNowPlaying() {
    SshResult coreResult = ssh.runCommand("cat /tmp/CORENAME 2>/dev/null");
    SshResult activeGameResult = ssh.runCommand("cat /tmp/ACTIVEGAME 2>/dev/null");

    std::string activeGame = activeGameResult.success ? trim(activeGameResult.output) : "";
    if (activeGame.empty()) {
        SshResult fullPathResult = ssh.runCommand("cat /tmp/FULLPATH 2>/dev/null");
        activeGame = fullPathResult.success ? trim(fullPathResult.output) : "";
    }

    std::string core = coreResult.success ? normalizeCoreName(coreResult.output) : "";
    std::string upper = core;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    if (!core.empty() && upper != "UNKNOWN" && upper != "MENU" && !activeGame.empty()) {
        nowPlaying = core + " | " + prettifyGameName(activeGame);
    } else {
        nowPlaying.clear();
    }
}

void App::toggleSmb() {
    if (!ssh.isConnected()) {
        lastMessage = "No active SSH connection.";
        return;
    }

    refreshSmb();
    const bool enable = smbStatus != "Enabled";
    const char* command = enable
        ? "if [ -f /media/fat/linux/_samba.sh ]; then mv /media/fat/linux/_samba.sh /media/fat/linux/samba.sh; fi"
        : "if [ -f /media/fat/linux/samba.sh ]; then mv /media/fat/linux/samba.sh /media/fat/linux/_samba.sh; fi";

    SshResult result = ssh.runCommand(command);
    if (result.success) {
        refreshSmb();
        lastMessage = enable ? "SMB enabled. A reboot is required." : "SMB disabled. A reboot is required.";
    } else {
        lastMessage = result.error.empty() ? "Unable to change SMB status." : result.error;
    }
}

void App::returnToMenu() {
    lastMessage = runCommandMessage("echo \"load_core /media/fat/menu.rbf\" > /dev/MiSTer_cmd");
    refreshNowPlaying();
}

void App::reboot() {
    if (!ssh.isConnected()) {
        lastMessage = "No active SSH connection.";
        return;
    }
    if (!confirm("Confirm Reboot", "Are you sure you want to reboot the MiSTer?")) return;

    SshResult result = ssh.runCommand("nohup /sbin/reboot >/dev/null 2>&1 &");
    ssh.disconnect();
    status = "Disconnected";
    sdStorage = "Rebooting...";
    usbStorage = "Rebooting...";
    smbStatus = "Rebooting...";
    nowPlaying.clear();
    lastMessage = result.success ? "Reboot command sent." : "Reboot command may have failed.";
}

std::string App::formatDfLine(const std::string& line) {
    std::vector<std::string> parts = splitWords(line);
    if (parts.size() < 5) return trim(line.empty() ? "Unknown" : line);
    return parts[3] + " free of " + parts[1] + " (" + parts[4] + " used)";
}

std::string App::normalizeCoreName(std::string core) {
    core = trim(core);
    if (core.empty()) return "Unknown";
    std::replace(core.begin(), core.end(), '_', ' ');

    std::string compact;
    for (char c : core) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) compact.push_back(::toupper(c));
    }

    if (compact == "TGFX16") return "TurboGrafx-16";
    if (compact == "PCECD") return "PC Engine CD";
    if (compact == "SMS") return "Master System";
    if (compact == "GENESIS") return "Genesis";
    if (compact == "MEGADRIVE") return "Mega Drive";
    if (compact == "NES") return "NES";
    if (compact == "SNES") return "SNES";
    if (compact == "GBA") return "Game Boy Advance";
    if (compact == "GBC") return "Game Boy Color";
    if (compact == "GB") return "Game Boy";
    if (compact == "PSX") return "PlayStation";
    if (compact == "AO486") return "ao486";

    return core;
}

std::string App::prettifyGameName(const std::string& path) {
    std::string value = trim(path);
    while (!value.empty() && (value.back() == '/' || value.back() == '\\')) value.pop_back();

    size_t slash = value.find_last_of("/\\");
    std::string name = slash == std::string::npos ? value : value.substr(slash + 1);
    size_t dot = name.find_last_of('.');
    if (dot != std::string::npos) name = name.substr(0, dot);

    const std::vector<std::string> tokens = {"(Disc 1)", "(Disc 2)", "(Disc 3)", "(USA)", "(Europe)", "(Japan)", "[!]", "_"};
    for (const std::string& token : tokens) {
        size_t pos;
        while ((pos = name.find(token)) != std::string::npos) {
            name.replace(pos, token.size(), token == "_" ? " " : "");
        }
    }

    return trim(name.empty() ? "Unknown" : name);
}