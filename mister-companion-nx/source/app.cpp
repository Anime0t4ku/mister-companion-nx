#include "app.hpp"

#include <switch.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <sstream>

static const char* RemoteScriptPath = "/media/fat/Scripts/companion_remote.sh";
static const char* RemoteScriptUrl = "https://raw.githubusercontent.com/Anime0t4ku/mister-companion/main/mister-companion/assets/companion_remote.sh";

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

static std::string safeText(const std::string& value, const std::string& fallback = "Not set") {
    return value.empty() ? fallback : value;
}

static bool contains(const std::string& value, const std::string& needle) {
    return value.find(needle) != std::string::npos;
}

static std::string remoteManageCommand(const std::string& action) {
    return std::string(RemoteScriptPath) + " " + action + " --unattended";
}

void App::run() {
    config = ConfigStore::load();
    if (!ui.initialize()) return;

    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        padUpdate(&pad);
        const u64 down = padGetButtonsDown(&pad);
        const u64 up = padGetButtonsUp(&pad);
        const u64 held = padGetButtons(&pad);

        if (passthroughActive) {
            handlePassthroughInput(down, up, held);
            draw();
            continue;
        }

        if (down & HidNpadButton_Plus) break;

        handleInput(down);
        draw();
    }

    if (passthroughActive) stopPassthrough();
    remote.disconnect();
    ui.shutdown();
}

void App::drawHeader() {
    ui.clear(UiRenderer::rgb(12, 10, 20));
    ui.fillRect(0, 0, UiRenderer::Width, 92, UiRenderer::rgb(20, 16, 34));
    ui.fillRect(0, 90, UiRenderer::Width, 4, UiRenderer::rgb(143, 84, 255));

    ui.drawText(42, 30, "MISTER COMPANION NX", UiRenderer::rgb(248, 245, 255), 3);
    ui.drawStatusPill(UiRenderer::Width - 280, 28, ssh.isConnected() ? "CONNECTED" : "DISCONNECTED", ssh.isConnected());

    ui.drawTab(40, 110, 240, "CONNECTION", tab == Tab::Connection);
    ui.drawTab(300, 110, 200, "DEVICE", tab == Tab::Device);
    ui.drawTab(520, 110, 200, "REMOTE", tab == Tab::Remote);
}

void App::draw() {
    ui.beginFrame();

    if (passthroughActive) {
        drawPassthrough();
        ui.endFrame();
        return;
    }

    drawHeader();

    if (tab == Tab::Connection) drawConnection();
    else if (tab == Tab::Device) drawDevice();
    else drawRemote();

    ui.drawMessage(lastMessage);
    ui.drawFooter("UP/DOWN SELECT    A CONFIRM/EDIT    L/R SWITCH TAB    + EXIT");
    ui.endFrame();
}

void App::drawConnection() {
    ui.drawCard(40, 176, 580, 400, "CONNECTION");
    ui.drawCard(660, 176, 580, 400, "STATUS");

    ui.drawButton(76, 252, 508, 60, "HOST  " + safeText(config.host), selected == 0);
    ui.drawButton(76, 334, 508, 60, "USER  " + safeText(config.username), selected == 1);
    ui.drawButton(76, 416, 508, 60, config.password.empty() ? "PASSWORD  NOT SET" : "PASSWORD  ********", selected == 2);
    ui.drawButton(76, 498, 508, 60, ssh.isConnected() ? "DISCONNECT" : "CONNECT", selected == 3);

    ui.drawText(696, 254, "CONNECTION STATUS", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(696, 296, status, UiRenderer::rgb(248, 245, 255), 3);

    ui.drawText(696, 380, "TIP", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(696, 420, "USE ROOT / 1 FOR DEFAULT MISTER SSH", UiRenderer::rgb(218, 208, 238), 2);
}

void App::drawDevice() {
    ui.drawCard(40, 176, 580, 400, "DEVICE STATUS");
    ui.drawCard(660, 176, 580, 400, "DEVICE ACTIONS");

    ui.drawText(76, 246, "MISTER", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(220, 246, safeText(config.host), UiRenderer::rgb(248, 245, 255), 2);

    ui.drawText(76, 294, "STATUS", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(220, 294, ssh.isConnected() ? "CONNECTED" : "DISCONNECTED", ssh.isConnected() ? UiRenderer::rgb(112, 232, 165) : UiRenderer::rgb(232, 130, 160), 2);

    ui.drawText(76, 342, "SD", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(220, 342, sdStorage, UiRenderer::rgb(248, 245, 255), 2);

    ui.drawText(76, 390, "USB", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(220, 390, usbStorage, UiRenderer::rgb(248, 245, 255), 2);

    ui.drawText(76, 438, "SMB", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(220, 438, smbStatus, UiRenderer::rgb(248, 245, 255), 2);

    if (!nowPlaying.empty()) {
        ui.drawText(76, 500, "NOW PLAYING", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawText(76, 532, nowPlaying, UiRenderer::rgb(248, 245, 255), 2);
    }

    ui.drawButton(696, 252, 508, 60, "REFRESH DEVICE INFO", selected == 0);
    ui.drawButton(696, 334, 508, 60, "TOGGLE SMB STARTUP", selected == 1);
    ui.drawButton(696, 416, 508, 60, "RETURN TO MISTER MENU", selected == 2);
    ui.drawButton(696, 498, 508, 60, "REBOOT MISTER", selected == 3, true);
}

void App::drawRemote() {
    ui.drawCard(40, 176, 580, 400, "REMOTE STATUS");
    ui.drawCard(660, 176, 580, 400, "REMOTE ACTIONS");

    ui.drawText(76, 246, "MISTER", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(260, 246, safeText(config.host), UiRenderer::rgb(248, 245, 255), 2);

    ui.drawText(76, 294, "DAEMON", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(260, 294, remoteInstalled, UiRenderer::rgb(248, 245, 255), 2);

    ui.drawText(76, 342, "STATUS", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(260, 342, remoteRunning == "Yes" ? "RUNNING" : remoteRunning == "No" ? "STOPPED" : remoteRunning, remoteRunning == "Yes" ? UiRenderer::rgb(112, 232, 165) : UiRenderer::rgb(232, 130, 160), 2);

    ui.drawText(76, 390, "START ON BOOT", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(260, 390, remoteStartup, UiRenderer::rgb(248, 245, 255), 2);

    ui.drawText(76, 438, "PASSTHROUGH", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(260, 438, passthroughActive ? "ENABLED" : "DISABLED", passthroughActive ? UiRenderer::rgb(112, 232, 165) : UiRenderer::rgb(218, 208, 238), 2);

    ui.drawText(76, 506, "PASSTHROUGH EXIT", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(76, 538, "PRESS L3 AND R3 TOGETHER", UiRenderer::rgb(248, 245, 255), 2);

    const bool installed = remoteInstalled == "Yes";
    const bool running = remoteRunning == "Yes";
    const bool startup = remoteStartup == "Enabled";

    if (!installed) {
        ui.drawButton(696, 280, 508, 60, "INSTALL / UPDATE DAEMON", selected == 0);
        ui.drawButton(696, 362, 508, 60, "REFRESH REMOTE STATUS", selected == 1);
        return;
    }

    if (running) {
        ui.drawButton(696, 238, 508, 60, "START PASSTHROUGH MODE", selected == 0);
        ui.drawButton(696, 320, 508, 60, "STOP REMOTE DAEMON", selected == 1);
        ui.drawButton(696, 402, 508, 60, startup ? "DISABLE START ON BOOT" : "ENABLE START ON BOOT", selected == 2);
        ui.drawButton(696, 484, 508, 60, "REFRESH REMOTE STATUS", selected == 3);
        return;
    }

    ui.drawButton(696, 238, 508, 60, "START REMOTE DAEMON", selected == 0);
    ui.drawButton(696, 320, 508, 60, startup ? "DISABLE START ON BOOT" : "ENABLE START ON BOOT", selected == 1);
    ui.drawButton(696, 402, 508, 60, "UNINSTALL REMOTE DAEMON", selected == 2, true);
    ui.drawButton(696, 484, 508, 60, "REFRESH REMOTE STATUS", selected == 3);
}

void App::drawPassthrough() {
    ui.clear(UiRenderer::rgb(12, 10, 20));
    ui.fillRect(0, 0, UiRenderer::Width, UiRenderer::Height, UiRenderer::rgb(12, 10, 20));
    ui.fillRect(0, 0, UiRenderer::Width, 96, UiRenderer::rgb(20, 16, 34));
    ui.fillRect(0, 94, UiRenderer::Width, 4, UiRenderer::rgb(143, 84, 255));

    ui.drawText(42, 32, "REMOTE PASSTHROUGH ACTIVE", UiRenderer::rgb(248, 245, 255), 3);
    ui.drawStatusPill(UiRenderer::Width - 280, 28, remote.isConnected() ? "REMOTE READY" : "REMOTE LOST", remote.isConnected());

    ui.drawCard(220, 180, 840, 340, "SWITCH CONTROLS ARE FORWARDED TO MISTER");
    ui.drawText(280, 285, "USE THE SWITCH PHYSICAL BUTTONS TO CONTROL MISTER", UiRenderer::rgb(248, 245, 255), 2);
    ui.drawText(280, 345, "PRESS BOTH ANALOG STICKS AT THE SAME TIME", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(280, 390, "L3 + R3 EXITS PASSTHROUGH MODE", UiRenderer::rgb(248, 245, 255), 3);

    ui.drawFooter("PASSTHROUGH MODE    L3 + R3 EXIT    RELEASE ALL ON EXIT");
}

void App::handleInput(u64 buttons) {
    if (buttons & HidNpadButton_L) {
        if (tab == Tab::Connection) tab = Tab::Remote;
        else if (tab == Tab::Device) tab = Tab::Connection;
        else tab = Tab::Device;
        selected = 0;
        return;
    }
    if (buttons & HidNpadButton_R) {
        if (tab == Tab::Connection) tab = Tab::Device;
        else if (tab == Tab::Device) tab = Tab::Remote;
        else tab = Tab::Connection;
        selected = 0;
        if (tab == Tab::Remote && remoteInstalled == "Not checked") refreshRemoteStatus();
        return;
    }

    if (tab == Tab::Connection) handleConnectionInput(buttons);
    else if (tab == Tab::Device) handleDeviceInput(buttons);
    else handleRemoteInput(buttons);
}

void App::handleConnectionInput(u64 buttons) {
    if (buttons & HidNpadButton_Up) selected = (selected + 3) % 4;
    if (buttons & HidNpadButton_Down) selected = (selected + 1) % 4;
    if (!(buttons & HidNpadButton_A)) return;

    switch (selected) {
        case 0: editText("MiSTer IP or hostname", config.host); break;
        case 1: editText("SSH username", config.username); break;
        case 2: editText("SSH password", config.password, true); break;
        case 3: connectOrDisconnect(); break;
    }
}

void App::handleDeviceInput(u64 buttons) {
    if (buttons & HidNpadButton_Up) selected = (selected + 3) % 4;
    if (buttons & HidNpadButton_Down) selected = (selected + 1) % 4;
    if (!(buttons & HidNpadButton_A)) return;

    switch (selected) {
        case 0: refreshDevice(); break;
        case 1: toggleSmb(); break;
        case 2: returnToMenu(); break;
        case 3: reboot(); break;
    }
}

void App::handleRemoteInput(u64 buttons) {
    const bool installed = remoteInstalled == "Yes";
    const bool running = remoteRunning == "Yes";
    const int actionCount = installed ? 4 : 2;

    if (buttons & HidNpadButton_Up) selected = (selected + actionCount - 1) % actionCount;
    if (buttons & HidNpadButton_Down) selected = (selected + 1) % actionCount;
    if (!(buttons & HidNpadButton_A)) return;

    if (!installed) {
        switch (selected) {
            case 0: installRemoteDaemon(); break;
            case 1: refreshRemoteStatus(); break;
        }
        return;
    }

    if (running) {
        switch (selected) {
            case 0: startPassthrough(); break;
            case 1: stopRemoteDaemon(); break;
            case 2: toggleRemoteStartup(); break;
            case 3: refreshRemoteStatus(); break;
        }
        return;
    }

    switch (selected) {
        case 0: startRemoteDaemon(); break;
        case 1: toggleRemoteStartup(); break;
        case 2: uninstallRemoteDaemon(); break;
        case 3: refreshRemoteStatus(); break;
    }
}

void App::handlePassthroughInput(u64 down, u64 up, u64 held) {
    if ((held & HidNpadButton_StickL) && (held & HidNpadButton_StickR)) {
        stopPassthrough();
        return;
    }

    sendPassthroughButton(HidNpadButton_Up, down, "dpad", "up", "down");
    sendPassthroughButton(HidNpadButton_Down, down, "dpad", "down", "down");
    sendPassthroughButton(HidNpadButton_Left, down, "dpad", "left", "down");
    sendPassthroughButton(HidNpadButton_Right, down, "dpad", "right", "down");

    sendPassthroughButton(HidNpadButton_Up, up, "dpad", "up", "up");
    sendPassthroughButton(HidNpadButton_Down, up, "dpad", "down", "up");
    sendPassthroughButton(HidNpadButton_Left, up, "dpad", "left", "up");
    sendPassthroughButton(HidNpadButton_Right, up, "dpad", "right", "up");

    sendPassthroughButton(HidNpadButton_A, down, "button", "b", "down");
    sendPassthroughButton(HidNpadButton_B, down, "button", "a", "down");
    sendPassthroughButton(HidNpadButton_X, down, "button", "y", "down");
    sendPassthroughButton(HidNpadButton_Y, down, "button", "x", "down");
    sendPassthroughButton(HidNpadButton_L, down, "button", "l", "down");
    sendPassthroughButton(HidNpadButton_R, down, "button", "r", "down");
    sendPassthroughButton(HidNpadButton_ZL, down, "button", "zl", "down");
    sendPassthroughButton(HidNpadButton_ZR, down, "button", "zr", "down");
    sendPassthroughButton(HidNpadButton_Minus, down, "button", "select", "down");
    sendPassthroughButton(HidNpadButton_Plus, down, "button", "start", "down");

    sendPassthroughButton(HidNpadButton_A, up, "button", "b", "up");
    sendPassthroughButton(HidNpadButton_B, up, "button", "a", "up");
    sendPassthroughButton(HidNpadButton_X, up, "button", "y", "up");
    sendPassthroughButton(HidNpadButton_Y, up, "button", "x", "up");
    sendPassthroughButton(HidNpadButton_L, up, "button", "l", "up");
    sendPassthroughButton(HidNpadButton_R, up, "button", "r", "up");
    sendPassthroughButton(HidNpadButton_ZL, up, "button", "zl", "up");
    sendPassthroughButton(HidNpadButton_ZR, up, "button", "zr", "up");
    sendPassthroughButton(HidNpadButton_Minus, up, "button", "select", "up");
    sendPassthroughButton(HidNpadButton_Plus, up, "button", "start", "up");
}

void App::sendPassthroughButton(u64 mask, u64 buttons, const std::string& control, const std::string& name, const std::string& action) {
    if (!passthroughActive) return;
    if (!(buttons & mask)) return;
    std::string message;
    if (!remote.sendController(control, name, action, message)) {
        passthroughActive = false;
        lastMessage = message;
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
    int choice = 1;
    bool inputReleased = false;

    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, UiRenderer::Height, UiRenderer::rgb(12, 10, 20));
        ui.drawCard(300, 190, 680, 300, title);
        ui.drawText(350, 280, body, UiRenderer::rgb(248, 245, 255), 2);
        ui.drawButton(350, 370, 260, 58, "YES", choice == 0, true);
        ui.drawButton(670, 370, 260, 58, "NO", choice == 1);
        ui.drawFooter("LEFT/RIGHT SELECT    A CONFIRM    B CANCEL");
        ui.endFrame();

        padUpdate(&pad);
        const u64 held = padGetButtons(&pad);
        const u64 buttons = padGetButtonsDown(&pad);

        if (!inputReleased) {
            const u64 confirmButtons = HidNpadButton_A | HidNpadButton_B | HidNpadButton_Left | HidNpadButton_Right;
            if ((held & confirmButtons) == 0) inputReleased = true;
            continue;
        }

        if (buttons & HidNpadButton_Left) choice = 0;
        if (buttons & HidNpadButton_Right) choice = 1;
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
    if (!ssh.isConnected()) return "No active MiSTer connection.";
    SshResult result = ssh.runCommand(command);
    if (result.success) return result.output.empty() ? "Command completed." : result.output;
    return result.error.empty() ? "Command failed." : result.error;
}

void App::refreshDevice() {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
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
        lastMessage = "No active MiSTer connection.";
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
        lastMessage = "No active MiSTer connection.";
        return;
    }
    if (!confirm("CONFIRM REBOOT", "ARE YOU SURE YOU WANT TO REBOOT THE MISTER?")) return;

    SshResult result = ssh.runCommand("nohup /sbin/reboot >/dev/null 2>&1 &");
    ssh.disconnect();
    status = "Disconnected";
    sdStorage = "Rebooting...";
    usbStorage = "Rebooting...";
    smbStatus = "Rebooting...";
    nowPlaying.clear();
    lastMessage = result.success ? "Reboot command sent." : "Reboot command may have failed.";
}

void App::refreshRemoteStatus() {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }

    const std::string command =
        "if [ -x /media/fat/Scripts/companion_remote.sh ]; then "
        "/media/fat/Scripts/companion_remote.sh status --unattended; "
        "else "
        "echo SCRIPT_INSTALLED=0; "
        "echo DAEMON_INSTALLED=0; "
        "echo DAEMON_RUNNING=0; "
        "echo PORT_LISTENING=0; "
        "echo START_ON_BOOT=0; "
        "fi";

    SshResult result = ssh.runCommand(command);
    if (!result.success) {
        lastMessage = result.error.empty() ? "Unable to read remote status." : result.error;
        return;
    }

    remoteInstalled = contains(result.output, "DAEMON_INSTALLED=1") || contains(result.output, "SCRIPT_INSTALLED=1") ? "Yes" : "No";
    remoteRunning = contains(result.output, "DAEMON_RUNNING=1") || contains(result.output, "PORT_LISTENING=1") ? "Yes" : "No";
    remoteStartup = contains(result.output, "START_ON_BOOT=1") ? "Enabled" : "Disabled";
    lastMessage = "Remote status refreshed.";
}

void App::installRemoteDaemon() {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }

    const std::string command =
        "mkdir -p /media/fat/Scripts && "
        "(command -v curl >/dev/null 2>&1 && curl -L -o " + std::string(RemoteScriptPath) + " " + RemoteScriptUrl + " || "
        "command -v wget >/dev/null 2>&1 && wget -O " + std::string(RemoteScriptPath) + " " + RemoteScriptUrl + ") && "
        "chmod +x " + std::string(RemoteScriptPath) + " && " +
        remoteManageCommand("install");

    lastMessage = runCommandMessage(command);
    refreshRemoteStatus();
}

void App::startRemoteDaemon() {
    lastMessage = runCommandMessage(remoteManageCommand("start"));
    refreshRemoteStatus();
}

void App::stopRemoteDaemon() {
    std::string message;
    if (remote.isConnected()) remote.releaseAll(message);
    remote.disconnect();
    lastMessage = runCommandMessage(remoteManageCommand("stop"));
    refreshRemoteStatus();
}

void App::toggleRemoteStartup() {
    const bool enabled = remoteStartup == "Enabled";
    lastMessage = runCommandMessage(remoteManageCommand(enabled ? "disable-boot" : "enable-boot"));
    refreshRemoteStatus();
}

void App::uninstallRemoteDaemon() {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }
    if (!confirm("UNINSTALL REMOTE", "REMOVE THE COMPANION REMOTE DAEMON?")) return;

    std::string message;
    if (remote.isConnected()) remote.releaseAll(message);
    remote.disconnect();
    lastMessage = runCommandMessage(remoteManageCommand("uninstall"));
    refreshRemoteStatus();
}


void App::startPassthrough() {
    std::string message;
    if (!remote.isConnected()) {
        if (!remote.connect(config.host, message)) {
                lastMessage = message;
            return;
        }
    }

    passthroughActive = true;
    lastMessage.clear();
}

void App::stopPassthrough() {
    std::string message;
    if (remote.isConnected()) remote.releaseAll(message);
    remote.disconnect();
    passthroughActive = false;
    lastMessage = "Passthrough stopped. All inputs released.";
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
