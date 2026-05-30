#include "app.hpp"

#include <switch.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <sstream>

static const char* RemoteScriptPath = "/media/fat/Scripts/companion_remote.sh";
static const char* RemoteScriptUrl = "https://raw.githubusercontent.com/Anime0t4ku/mister-companion/main/mister-companion/assets/companion_remote.sh";

static constexpr int ScriptCount = 11;
static const char* UpdateAllPath = "/media/fat/Scripts/update_all.sh";
static const char* ZaparooPath = "/media/fat/Scripts/zaparoo.sh";
static const char* MigrateSdPath = "/media/fat/Scripts/migrate_sd.sh";
static const char* CifsMountPath = "/media/fat/Scripts/cifs_mount.sh";
static const char* CifsUmountPath = "/media/fat/Scripts/cifs_umount.sh";
static const char* CifsConfigPath = "/media/fat/Scripts/cifs_mount.ini";
static const char* AutoTimePath = "/media/fat/Scripts/auto_time.sh";
static const char* CdGameOrganizerPath = "/media/fat/Scripts/cd_game_organizer.sh";
static const char* DavBrowserPath = "/media/fat/Scripts/dav_browser.sh";
static const char* DavBrowserConfigPath = "/media/fat/Scripts/.config/dav_browser/dav_browser.ini";
static const char* FtpSaveSyncPath = "/media/fat/Scripts/ftp_save_sync.sh";
static const char* FtpSaveSyncConfigPath = "/media/fat/Scripts/.config/ftp_save_sync/ftp_save_sync.ini";
static const char* StaticWallpaperPath = "/media/fat/Scripts/static_wallpaper.sh";
static const char* SyncthingPath = "/media/fat/Scripts/syncthing.sh";
static const char* RaViewerPath = "/media/fat/Scripts/ra_viewer.sh";
static const char* RaViewerConfigPath = "/media/fat/Scripts/.config/ra_viewer/config.ini";
static const char* UserStartupPath = "/media/fat/linux/user-startup.sh";

static const char* UrlMigrateSd = "https://raw.githubusercontent.com/Natrox/MiSTer_Utils_Natrox/main/scripts/migrate_sd.sh";
static const char* UrlCifsMount = "https://raw.githubusercontent.com/MiSTer-devel/Scripts_MiSTer/master/cifs_mount.sh";
static const char* UrlCifsUmount = "https://raw.githubusercontent.com/MiSTer-devel/Scripts_MiSTer/master/cifs_umount.sh";
static const char* UrlAutoTime = "https://raw.githubusercontent.com/Anime0t4ku/0t4ku-mister-scripts/main/Scripts/auto_time.sh";
static const char* UrlCdGameOrganizer = "https://raw.githubusercontent.com/Anime0t4ku/0t4ku-mister-scripts/main/Scripts/cd_game_organizer.sh";
static const char* UrlDavBrowser = "https://raw.githubusercontent.com/Anime0t4ku/0t4ku-mister-scripts/main/Scripts/dav_browser.sh";
static const char* UrlFtpSaveSync = "https://raw.githubusercontent.com/Anime0t4ku/0t4ku-mister-scripts/refs/heads/main/Scripts/ftp_save_sync.sh";
static const char* UrlStaticWallpaper = "https://raw.githubusercontent.com/Anime0t4ku/0t4ku-mister-scripts/main/Scripts/static_wallpaper.sh";
static const char* UrlSyncthingScript = "https://raw.githubusercontent.com/Anime0t4ku/0t4ku-mister-scripts/main/Scripts/syncthing.sh";
static const char* UrlRaViewer = "https://raw.githubusercontent.com/Anime0t4ku/0t4ku-mister-scripts/main/Scripts/ra_viewer.sh";

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

static std::string shellQuote(const std::string& value) {
    std::string out = "'";
    for (char c : value) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    out += "'";
    return out;
}

static std::string downloadCommand(const std::string& url, const std::string& path) {
    return "mkdir -p /media/fat/Scripts /media/fat/Scripts/.config/update_all && "
           "if ! command -v wget >/dev/null 2>&1; then echo wget not found.; exit 1; fi; "
           "wget --no-check-certificate -O " + path + " " + shellQuote(url) + " && "
           "test -s " + path + " && "
           "chmod +x " + path;
}


static std::vector<std::string> splitLines(const std::string& value) {
    std::vector<std::string> lines;
    std::stringstream stream(value);
    std::string line;
    while (std::getline(stream, line)) lines.push_back(line);
    return lines;
}

static std::string joinLines(const std::vector<std::string>& lines) {
    std::string out;
    for (const std::string& line : lines) {
        out += line;
        out += "\n";
    }
    return out;
}

static std::vector<std::string> removeSectionFromLines(const std::vector<std::string>& lines, const std::string& section) {
    std::vector<std::string> out;
    bool skip = false;
    for (const std::string& line : lines) {
        std::string stripped = trim(line);
        if (stripped.size() >= 2 && stripped.front() == '[' && stripped.back() == ']') {
            std::string current = stripped.substr(1, stripped.size() - 2);
            skip = current == section;
        }
        if (!skip) out.push_back(line);
    }
    return out;
}

static std::vector<std::string> normalizeIniLines(std::vector<std::string> lines) {
    while (!lines.empty() && trim(lines.front()).empty()) lines.erase(lines.begin());
    while (!lines.empty() && trim(lines.back()).empty()) lines.pop_back();

    std::vector<std::string> out;
    bool previousBlank = false;
    for (const std::string& line : lines) {
        bool blank = trim(line).empty();
        if (blank && previousBlank) continue;
        out.push_back(line);
        previousBlank = blank;
    }
    return out;
}

static void appendSection(std::vector<std::string>& lines, const std::vector<std::string>& sectionLines) {
    lines = normalizeIniLines(lines);
    if (!lines.empty()) lines.push_back("");
    for (const std::string& line : sectionLines) lines.push_back(line);
}

static std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

static bool sectionEnabledInText(const std::string& text, const std::string& section) {
    for (const std::string& line : splitLines(text)) {
        std::string stripped = trim(line);
        if (stripped.empty() || stripped.front() == '#' || stripped.front() == ';') continue;
        if (stripped.size() >= 2 && stripped.front() == '[' && stripped.back() == ']') {
            if (stripped.substr(1, stripped.size() - 2) == section) return true;
        }
    }
    return false;
}

static bool iniBoolValue(const std::string& text, const std::string& key, bool defaultValue = false) {
    const std::string wanted = toLower(key);
    for (const std::string& line : splitLines(text)) {
        std::string stripped = trim(line);
        if (stripped.empty() || stripped.front() == '#' || stripped.front() == ';') continue;
        size_t eq = stripped.find('=');
        if (eq == std::string::npos) continue;
        if (toLower(trim(stripped.substr(0, eq))) != wanted) continue;
        std::string value = toLower(trim(stripped.substr(eq + 1)));
        return value == "true" || value == "1" || value == "yes" || value == "on";
    }
    return defaultValue;
}

static std::string extractSectionValue(const std::string& text, const std::string& section, const std::string& key) {
    bool inSection = false;
    for (const std::string& line : splitLines(text)) {
        std::string stripped = trim(line);
        if (stripped.size() >= 2 && stripped.front() == '[' && stripped.back() == ']') {
            inSection = stripped.substr(1, stripped.size() - 2) == section;
            continue;
        }
        if (!inSection || stripped.empty() || stripped.front() == '#' || stripped.front() == ';') continue;
        size_t eq = stripped.find('=');
        if (eq == std::string::npos) continue;
        if (trim(stripped.substr(0, eq)) == key) return trim(stripped.substr(eq + 1));
    }
    return "";
}

static bool jsonBoolValue(const std::string& json, const std::string& key) {
    std::string marker = "\"" + key + "\"";
    size_t pos = json.find(marker);
    if (pos == std::string::npos) return false;
    size_t colon = json.find(':', pos + marker.size());
    if (colon == std::string::npos) return false;
    size_t value = json.find_first_not_of(" \t\r\n", colon + 1);
    if (value == std::string::npos) return false;
    return json.compare(value, 4, "true") == 0;
}

static std::string setJsonBool(std::string json, const std::string& key, bool enabled) {
    if (trim(json).empty()) json = "{\n}\n";
    std::string marker = "\"" + key + "\"";
    std::string value = enabled ? "true" : "false";
    size_t pos = json.find(marker);
    if (pos != std::string::npos) {
        size_t colon = json.find(':', pos + marker.size());
        if (colon != std::string::npos) {
            size_t start = json.find_first_not_of(" \t\r\n", colon + 1);
            if (start != std::string::npos) {
                if (json.compare(start, 4, "true") == 0) {
                    json.replace(start, 4, value);
                    return json;
                }
                if (json.compare(start, 5, "false") == 0) {
                    json.replace(start, 5, value);
                    return json;
                }
            }
        }
    }

    size_t close = json.find_last_of('}');
    if (close == std::string::npos) return "{\n    \"" + key + "\": " + value + "\n}\n";

    std::string before = json.substr(0, close);
    std::string after = json.substr(close);
    while (!before.empty() && (before.back() == ' ' || before.back() == '\t' || before.back() == '\r' || before.back() == '\n')) before.pop_back();

    bool hasExisting = before.find(':') != std::string::npos;
    if (hasExisting && before.back() != '{') before += ",";
    before += "\n    \"" + key + "\": " + value + "\n";
    return before + after;
}


static std::string simpleConfigValue(const std::string& text, const std::string& key) {
    const std::string wanted = toLower(trim(key));
    for (const std::string& rawLine : splitLines(text)) {
        std::string line = trim(rawLine);
        if (line.empty() || line.front() == '#' || line.front() == ';') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        if (toLower(trim(line.substr(0, eq))) != wanted) continue;
        std::string value = trim(line.substr(eq + 1));
        if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') || (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.size() - 2);
        }
        return value;
    }
    return "";
}

static std::string boolText(bool value) {
    return value ? "YES" : "NO";
}

static std::string dirnameOf(const std::string& path) {
    size_t pos = path.find_last_of('/');
    if (pos == std::string::npos || pos == 0) return "/";
    return path.substr(0, pos);
}

static std::string writeTextCommand(const std::string& path, const std::string& text) {
    return "mkdir -p " + shellQuote(dirnameOf(path)) + " && cat > " + shellQuote(path) + " <<'MC_EOF'\n" + text + "MC_EOF\n";
}

static std::string fileStatusCommand(const std::string& path) {
    return "test -f " + path + " && echo YES || echo NO";
}

static std::string execStatusCommand(const std::string& path) {
    return "test -x " + path + " && echo YES || echo NO";
}

static std::string statusValue(const std::vector<std::string>& lines, const std::string& key) {
    const std::string prefix = key + ":";
    for (const std::string& line : lines) {
        if (line.rfind(prefix, 0) == 0) {
            return trim(line.substr(prefix.size()));
        }
    }
    return "";
}

static bool statusYes(const std::vector<std::string>& lines, const std::string& key) {
    return statusValue(lines, key) == "YES";
}

static std::string remoteManageCommand(const std::string& action) {
    return std::string(RemoteScriptPath) + " " + action + " --unattended";
}

void App::run() {
    config = ConfigStore::load();
    cachedScriptStatus.assign(ScriptCount, std::vector<std::string>{"STATUS: NOT CHECKED"});
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

    ui.drawTab(40, 110, 190, "CONNECTION", tab == Tab::Connection);
    ui.drawTab(250, 110, 160, "DEVICE", tab == Tab::Device);
    ui.drawTab(430, 110, 160, "REMOTE", tab == Tab::Remote);
    ui.drawTab(610, 110, 170, "SCRIPTS", tab == Tab::Scripts);
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
    else if (tab == Tab::Remote) drawRemote();
    else drawScripts();

    ui.drawMessage(lastMessage);
    if (tab == Tab::Scripts) {
        ui.drawFooter("UP/DOWN SELECT    A CONFIRM/EDIT    L/R TABS    ZL/ZR SCRIPT    + EXIT");
    } else {
        ui.drawFooter("UP/DOWN SELECT    A CONFIRM/EDIT    L/R SWITCH TAB    + EXIT");
    }
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

    const bool actionsEnabled = ssh.isConnected();
    ui.drawButton(696, 252, 508, 60, "REFRESH DEVICE INFO", selected == 0 && actionsEnabled, false, !actionsEnabled);
    ui.drawButton(696, 334, 508, 60, "TOGGLE SMB STARTUP", selected == 1 && actionsEnabled, false, !actionsEnabled);
    ui.drawButton(696, 416, 508, 60, "RETURN TO MISTER MENU", selected == 2 && actionsEnabled, false, !actionsEnabled);
    ui.drawButton(696, 498, 508, 60, "REBOOT MISTER", selected == 3 && actionsEnabled, true, !actionsEnabled);
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

    const bool actionsEnabled = ssh.isConnected();
    const bool installed = remoteInstalled == "Yes";
    const bool running = remoteRunning == "Yes";
    const bool startup = remoteStartup == "Enabled";

    if (!installed) {
        ui.drawButton(696, 280, 508, 60, "INSTALL / UPDATE DAEMON", selected == 0 && actionsEnabled, false, !actionsEnabled);
        ui.drawButton(696, 362, 508, 60, "REFRESH REMOTE STATUS", selected == 1 && actionsEnabled, false, !actionsEnabled);
        return;
    }

    if (running) {
        ui.drawButton(696, 238, 508, 60, "START PASSTHROUGH MODE", selected == 0 && actionsEnabled, false, !actionsEnabled);
        ui.drawButton(696, 320, 508, 60, "STOP REMOTE DAEMON", selected == 1 && actionsEnabled, false, !actionsEnabled);
        ui.drawButton(696, 402, 508, 60, startup ? "DISABLE START ON BOOT" : "ENABLE START ON BOOT", selected == 2 && actionsEnabled, false, !actionsEnabled);
        ui.drawButton(696, 484, 508, 60, "REFRESH REMOTE STATUS", selected == 3 && actionsEnabled, false, !actionsEnabled);
        return;
    }

    ui.drawButton(696, 238, 508, 60, "START REMOTE DAEMON", selected == 0 && actionsEnabled, false, !actionsEnabled);
    ui.drawButton(696, 320, 508, 60, startup ? "DISABLE START ON BOOT" : "ENABLE START ON BOOT", selected == 1 && actionsEnabled, false, !actionsEnabled);
    ui.drawButton(696, 402, 508, 60, "UNINSTALL REMOTE DAEMON", selected == 2 && actionsEnabled, true, !actionsEnabled);
    ui.drawButton(696, 484, 508, 60, "REFRESH REMOTE STATUS", selected == 3 && actionsEnabled, false, !actionsEnabled);
}


void App::drawScripts() {
    ui.drawCard(40, 176, 580, 400, "SCRIPT STATUS");
    ui.drawCard(660, 176, 580, 400, "SCRIPT ACTIONS");

    ScriptId id = static_cast<ScriptId>(selectedScript);
    std::string title = scriptTitle(id);
    std::vector<std::string> statusLines = selectedScript >= 0 && selectedScript < static_cast<int>(cachedScriptStatus.size())
        ? cachedScriptStatus[selectedScript]
        : std::vector<std::string>{"STATUS: NOT CHECKED"};
    std::vector<std::string> actions = scriptActions(id);

    ui.drawText(76, 236, "SCRIPT", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(220, 236, title, UiRenderer::rgb(248, 245, 255), 3);
    ui.drawText(76, 290, "SUB TAB", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(220, 290, std::to_string(selectedScript + 1) + " / " + std::to_string(ScriptCount), UiRenderer::rgb(248, 245, 255), 2);

    int y = 346;
    for (const std::string& line : statusLines) {
        if (y > 530) break;
        ui.drawText(76, y, line, UiRenderer::rgb(218, 208, 238), 2);
        y += 42;
    }

    const bool actionsEnabled = ssh.isConnected();
    int actionY = 232;
    for (int i = 0; i < static_cast<int>(actions.size()); i++) {
        if (actionY > 520) break;
        const bool danger = actions[i].find("UNINSTALL") != std::string::npos || actions[i].find("REMOVE") != std::string::npos;
        ui.drawButton(696, actionY, 508, 54, actions[i], selected == i && actionsEnabled, danger, !actionsEnabled);
        actionY += 68;
    }
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
        if (tab == Tab::Connection) tab = Tab::Scripts;
        else if (tab == Tab::Device) tab = Tab::Connection;
        else if (tab == Tab::Remote) tab = Tab::Device;
        else tab = Tab::Remote;
        selected = 0;
        if (tab == Tab::Scripts) refreshCurrentScriptStatus();
        return;
    }
    if (buttons & HidNpadButton_R) {
        if (tab == Tab::Connection) tab = Tab::Device;
        else if (tab == Tab::Device) tab = Tab::Remote;
        else if (tab == Tab::Remote) tab = Tab::Scripts;
        else tab = Tab::Connection;
        selected = 0;
        if (tab == Tab::Remote && ssh.isConnected() && remoteInstalled == "Not checked") refreshRemoteStatus();
        if (tab == Tab::Scripts) refreshCurrentScriptStatus();
        return;
    }

    if (tab == Tab::Connection) handleConnectionInput(buttons);
    else if (tab == Tab::Device) handleDeviceInput(buttons);
    else if (tab == Tab::Remote) handleRemoteInput(buttons);
    else handleScriptsInput(buttons);
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

    if (!ssh.isConnected()) {
        lastMessage = "Connect to a MiSTer first.";
        return;
    }

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

    if (!ssh.isConnected()) {
        lastMessage = "Connect to a MiSTer first.";
        return;
    }

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


void App::handleScriptsInput(u64 buttons) {
    if (buttons & HidNpadButton_ZL) {
        selectedScript = (selectedScript + ScriptCount - 1) % ScriptCount;
        selected = 0;
        refreshCurrentScriptStatus();
        return;
    }
    if (buttons & HidNpadButton_ZR) {
        selectedScript = (selectedScript + 1) % ScriptCount;
        selected = 0;
        refreshCurrentScriptStatus();
        return;
    }

    std::vector<std::string> actions = scriptActions(static_cast<ScriptId>(selectedScript));
    const int actionCount = static_cast<int>(actions.size());
    if (actionCount <= 0) return;

    if (buttons & HidNpadButton_Up) selected = (selected + actionCount - 1) % actionCount;
    if (buttons & HidNpadButton_Down) selected = (selected + 1) % actionCount;
    if (!(buttons & HidNpadButton_A)) return;

    if (!ssh.isConnected()) {
        lastMessage = "Connect to a MiSTer first.";
        return;
    }

    executeScriptAction(static_cast<ScriptId>(selectedScript), selected);
    refreshCurrentScriptStatus();
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
        if (passthroughActive) stopPassthrough();
        remote.disconnect();
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
        "if ! command -v wget >/dev/null 2>&1; then echo wget not found.; exit 1; fi; "
        "wget --no-check-certificate -O " + std::string(RemoteScriptPath) + " " + RemoteScriptUrl + " && "
        "test -s " + std::string(RemoteScriptPath) + " && "
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

std::string App::scriptTitle(ScriptId id) const {
    switch (id) {
        case ScriptId::UpdateAll: return "UPDATE ALL";
        case ScriptId::Zaparoo: return "ZAPAROO";
        case ScriptId::MigrateSd: return "MIGRATE SD";
        case ScriptId::CifsMount: return "CIFS MOUNT";
        case ScriptId::AutoTime: return "AUTO TIME";
        case ScriptId::CdGameOrganizer: return "CD GAME ORGANIZER";
        case ScriptId::DavBrowser: return "DAV BROWSER";
        case ScriptId::FtpSaveSync: return "FTP SAVE SYNC";
        case ScriptId::StaticWallpaper: return "STATIC WALLPAPER";
        case ScriptId::Syncthing: return "SYNCTHING";
        case ScriptId::RaViewer: return "RA VIEWER";
    }
    return "SCRIPT";
}

std::vector<std::string> App::scriptActions(ScriptId id) {
    const int idx = static_cast<int>(id);
    const std::vector<std::string> lines = idx >= 0 && idx < static_cast<int>(cachedScriptStatus.size())
        ? cachedScriptStatus[idx]
        : std::vector<std::string>{};

    const bool installed = statusYes(lines, "INSTALLED");
    const bool config = statusYes(lines, "CONFIG");
    const bool startup = statusYes(lines, "START ON BOOT");

    switch (id) {
        case ScriptId::UpdateAll:
            if (installed) return {"RUN", "CONFIGURE", "UNINSTALL"};
            return {"INSTALL"};
        case ScriptId::Zaparoo:
            if (installed) return {startup ? "DISABLE START ON BOOT" : "ENABLE START ON BOOT", "UNINSTALL"};
            return {"INSTALL"};
        case ScriptId::MigrateSd:
            return installed ? std::vector<std::string>{"UNINSTALL"} : std::vector<std::string>{"INSTALL"};
        case ScriptId::CifsMount: {
            if (!installed) return {"INSTALL"};
            std::vector<std::string> actions = {"CONFIGURE", "MOUNT", "UNMOUNT"};
            if (config) actions.push_back("REMOVE CONFIG");
            actions.push_back("UNINSTALL");
            return actions;
        }
        case ScriptId::AutoTime:
            return installed ? std::vector<std::string>{"UNINSTALL"} : std::vector<std::string>{"INSTALL"};
        case ScriptId::CdGameOrganizer:
            return installed ? std::vector<std::string>{"UNINSTALL"} : std::vector<std::string>{"INSTALL"};
        case ScriptId::DavBrowser: {
            if (!installed) return {"INSTALL"};
            std::vector<std::string> actions = {"CONFIGURE"};
            if (config) actions.push_back("REMOVE CONFIG");
            actions.push_back("UNINSTALL");
            return actions;
        }
        case ScriptId::FtpSaveSync: {
            if (!installed) return {"INSTALL"};
            std::vector<std::string> actions = {"CONFIGURE", startup ? "DISABLE START ON BOOT" : "ENABLE START ON BOOT"};
            if (config) actions.push_back("REMOVE CONFIG");
            actions.push_back("UNINSTALL");
            return actions;
        }
        case ScriptId::StaticWallpaper:
            return installed ? std::vector<std::string>{"UNINSTALL"} : std::vector<std::string>{"INSTALL"};
        case ScriptId::Syncthing:
            if (installed) return {startup ? "DISABLE START ON BOOT" : "ENABLE START ON BOOT", "UNINSTALL"};
            return {"INSTALL"};
        case ScriptId::RaViewer: {
            if (!installed) return {"INSTALL"};
            std::vector<std::string> actions = {"EDIT CONFIG"};
            actions.push_back("UNINSTALL");
            return actions;
        }
    }
    return {};
}

void App::refreshCurrentScriptStatus() {
    if (selectedScript < 0 || selectedScript >= ScriptCount) return;
    if (!ssh.isConnected()) {
        cachedScriptStatus[selectedScript] = {"STATUS: DISCONNECTED"};
        return;
    }
    cachedScriptStatus[selectedScript] = scriptStatus(static_cast<ScriptId>(selectedScript));
}

std::vector<std::string> App::scriptStatus(ScriptId id) {
    if (!ssh.isConnected()) return {"STATUS: DISCONNECTED"};

    auto yesNo = [&](const std::string& cmd) -> std::string {
        SshResult r = ssh.runCommand(cmd);
        return r.success && contains(r.output, "YES") ? "YES" : "NO";
    };

    switch (id) {
        case ScriptId::UpdateAll:
            return {"INSTALLED: " + yesNo(execStatusCommand(UpdateAllPath))};
        case ScriptId::Zaparoo:
            return {"INSTALLED: " + yesNo(execStatusCommand(ZaparooPath)),
                    "START ON BOOT: " + yesNo("grep -F 'mrext/zaparoo' " + std::string(UserStartupPath) + " >/dev/null 2>&1 && echo YES || echo NO")};
        case ScriptId::MigrateSd:
            return {"INSTALLED: " + yesNo(execStatusCommand(MigrateSdPath))};
        case ScriptId::CifsMount:
            return {"INSTALLED: " + yesNo(execStatusCommand(CifsMountPath)),
                    "CONFIG: " + yesNo(fileStatusCommand(CifsConfigPath))};
        case ScriptId::AutoTime:
            return {"INSTALLED: " + yesNo(execStatusCommand(AutoTimePath))};
        case ScriptId::CdGameOrganizer:
            return {"INSTALLED: " + yesNo(execStatusCommand(CdGameOrganizerPath))};
        case ScriptId::DavBrowser:
            return {"INSTALLED: " + yesNo(execStatusCommand(DavBrowserPath)),
                    "CONFIG: " + yesNo(fileStatusCommand(DavBrowserConfigPath))};
        case ScriptId::FtpSaveSync:
            return {"INSTALLED: " + yesNo(execStatusCommand(FtpSaveSyncPath)),
                    "CONFIG: " + yesNo(fileStatusCommand(FtpSaveSyncConfigPath)),
                    "START ON BOOT: " + yesNo("grep -F 'ftp_save_sync_daemon.sh' " + std::string(UserStartupPath) + " >/dev/null 2>&1 && echo YES || echo NO")};
        case ScriptId::StaticWallpaper:
            return {"INSTALLED: " + yesNo(execStatusCommand(StaticWallpaperPath)),
                    "ACTIVE: " + yesNo("test -f /media/fat/menu.jpg -o -f /media/fat/menu.png && echo YES || echo NO")};
        case ScriptId::Syncthing:
            return {"INSTALLED: " + yesNo(execStatusCommand(SyncthingPath)),
                    "START ON BOOT: " + yesNo("grep -F 'syncthing_service.sh' " + std::string(UserStartupPath) + " >/dev/null 2>&1 && echo YES || echo NO")};
        case ScriptId::RaViewer:
            return {"INSTALLED: " + yesNo(execStatusCommand(RaViewerPath)),
                    "CONFIG: " + yesNo(fileStatusCommand(RaViewerConfigPath))};
    }
    return {"STATUS: UNKNOWN"};
}

void App::ensureScriptsDirs() {
    runCommandMessage("mkdir -p /media/fat/Scripts /media/fat/linux /media/fat/Scripts/.config/update_all /media/fat/Scripts/.config/dav_browser /media/fat/Scripts/.config/ftp_save_sync /media/fat/Scripts/.config/ra_viewer /media/fat/Scripts/.config/syncthing");
}

bool App::askField(const char* title, std::string& value, bool password) {
    std::string before = value;
    editText(title, value, password);
    return value != before || !value.empty();
}

bool App::askYesNo(const char* title, const char* body, bool defaultYes) {
    (void)defaultYes;
    return confirm(title, body);
}

void App::scriptInstall(const std::string& title, const std::string& path, const std::string& url) {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }
    ensureScriptsDirs();
    showStreamingCommandWindow("INSTALL " + title, downloadCommand(url, path), title + " installed.", title + " install failed.");
}

void App::scriptUninstall(const std::string& title, const std::string& command) {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }
    if (!confirm(("UNINSTALL " + title).c_str(), "ARE YOU SURE?")) return;
    lastMessage = runCommandMessage(command);
    if (lastMessage.empty()) lastMessage = title + " uninstalled.";
}

void App::showOutputWindow(const std::string& title, const std::string& output) {
    std::vector<std::string> lines;
    std::stringstream stream(output.empty() ? std::string("No output.") : output);
    std::string line;
    while (std::getline(stream, line)) {
        line = trim(line);
        if (line.empty()) {
            lines.push_back("");
            continue;
        }
        const size_t maxChars = 74;
        while (line.size() > maxChars) {
            lines.push_back(line.substr(0, maxChars));
            line = line.substr(maxChars);
        }
        lines.push_back(line);
    }
    if (lines.empty()) lines.push_back("No output.");

    int scroll = 0;
    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        padUpdate(&pad);
        const u64 down = padGetButtonsDown(&pad);
        if (down & (HidNpadButton_B | HidNpadButton_A | HidNpadButton_Plus)) break;
        if (down & HidNpadButton_Up) scroll = std::max(0, scroll - 1);
        if (down & HidNpadButton_Down) scroll = std::min(std::max(0, static_cast<int>(lines.size()) - 11), scroll + 1);

        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, 92, UiRenderer::rgb(20, 16, 34));
        ui.fillRect(0, 90, UiRenderer::Width, 4, UiRenderer::rgb(143, 84, 255));
        ui.drawText(42, 30, title, UiRenderer::rgb(248, 245, 255), 3);
        ui.drawCard(40, 124, 1200, 500, "OUTPUT");

        int y = 188;
        for (int i = scroll; i < static_cast<int>(lines.size()) && y <= 570; i++) {
            ui.drawText(76, y, lines[i], UiRenderer::rgb(218, 208, 238), 2);
            y += 34;
        }

        ui.drawFooter("UP/DOWN SCROLL    A/B CLOSE    + CLOSE");
        ui.endFrame();
    }
}

void App::showStreamingCommandWindow(const std::string& title, const std::string& command, const std::string& successMessage, const std::string& failureMessage) {
    std::vector<std::string> lines;
    lines.push_back("Starting...");
    int scroll = 0;
    bool completed = false;
    bool success = false;
    std::string finalError;
    std::string completionTitle = "RUNNING";

    auto appendText = [&](const std::string& text) {
        std::stringstream stream(text);
        std::string line;
        bool any = false;
        while (std::getline(stream, line)) {
            any = true;
            line = trim(line);
            const size_t maxChars = 74;
            if (line.empty()) {
                lines.push_back("");
            } else {
                while (line.size() > maxChars) {
                    lines.push_back(line.substr(0, maxChars));
                    line = line.substr(maxChars);
                }
                lines.push_back(line);
            }
        }
        if (!any && !text.empty()) lines.push_back(text);
        const int maxScroll = std::max(0, static_cast<int>(lines.size()) - 11);
        scroll = maxScroll;
    };

    auto drawStreamingWindow = [&]() {
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, 92, UiRenderer::rgb(20, 16, 34));
        ui.fillRect(0, 90, UiRenderer::Width, 4, UiRenderer::rgb(143, 84, 255));
        ui.drawText(42, 30, title, UiRenderer::rgb(248, 245, 255), 3);
        ui.drawCard(40, 124, 1200, 500, completed ? completionTitle : "RUNNING");

        int y = 188;
        for (int i = scroll; i < static_cast<int>(lines.size()) && y <= 570; i++) {
            ui.drawText(76, y, lines[i], UiRenderer::rgb(218, 208, 238), 2);
            y += 34;
        }

        ui.drawFooter(completed ? "UP/DOWN SCROLL    B CLOSE    A CLOSE" : "UP/DOWN SCROLL    PLEASE WAIT");
        ui.endFrame();
    };

    drawStreamingWindow();

    SshResult result = ssh.runCommandStreaming(command + " 2>&1", [&](const std::string& chunk) {
        appendText(chunk);

        PadState pad;
        padInitializeDefault(&pad);
        padUpdate(&pad);
        const u64 down = padGetButtonsDown(&pad);
        if (down & HidNpadButton_Up) scroll = std::max(0, scroll - 1);
        if (down & HidNpadButton_Down) scroll = std::min(std::max(0, static_cast<int>(lines.size()) - 11), scroll + 1);

        drawStreamingWindow();
    });

    success = result.success;
    completionTitle = success ? "FINISHED" : "FAILED";
    if (!result.error.empty()) finalError = result.error;
    appendText("");
    appendText(success ? "Finished successfully." : "Finished with errors.");
    appendText(success ? successMessage : failureMessage);
    if (!finalError.empty()) appendText(finalError);
    appendText("");
    appendText("Press B to close.");
    completed = true;
    drawStreamingWindow();

    PadState pad;
    padInitializeDefault(&pad);
    while (appletMainLoop()) {
        padUpdate(&pad);
        const u64 down = padGetButtonsDown(&pad);
        if (down & (HidNpadButton_B | HidNpadButton_A | HidNpadButton_Plus)) break;
        if (down & HidNpadButton_Up) scroll = std::max(0, scroll - 1);
        if (down & HidNpadButton_Down) scroll = std::min(std::max(0, static_cast<int>(lines.size()) - 11), scroll + 1);
        drawStreamingWindow();
    }

    lastMessage = success ? successMessage : failureMessage;
}

void App::runScriptCommand(const std::string& title, const std::string& command, bool confirmFirst) {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }
    if (confirmFirst && !confirm(title.c_str(), "ARE YOU SURE?")) return;
    lastMessage = runCommandMessage(command);
    if (lastMessage.empty()) lastMessage = title + " complete.";
}

void App::configureUpdateAll() {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }

    ensureScriptsDirs();

    const std::string mainPath = "/media/fat/downloader.ini";
    const std::string arcadePath = "/media/fat/downloader_arcade_roms_db.ini";
    const std::string biosPath = "/media/fat/downloader_bios_db.ini";
    const std::string manualsPath = "/media/fat/downloader_ajgowans_manualsdb.ini";
    const std::string jsonPath = "/media/fat/Scripts/.config/update_all/update_all.json";
    const std::string arcadeOrganizerPath = "/media/fat/Scripts/update_arcade-organizer.ini";

    struct ConfigChoice {
        std::string label;
        std::vector<std::string> labels;
        int selected = 0;
    };

    struct ConfigToggle {
        std::string label;
        bool enabled = false;
    };

    struct ConfigAction {
        std::string label;
    };

    struct ConfigItem {
        enum class Type { Category, Toggle, Choice, Action } type = Type::Toggle;
        int index = 0;
        std::string label;
        bool indent = false;
        std::string dependsOn;
    };

    SshResult mainResult = ssh.runCommand("cat /media/fat/downloader.ini 2>/dev/null || true");
    SshResult arcadeResult = ssh.runCommand("cat /media/fat/downloader_arcade_roms_db.ini 2>/dev/null || true");
    SshResult biosResult = ssh.runCommand("cat /media/fat/downloader_bios_db.ini 2>/dev/null || true");
    SshResult manualsResult = ssh.runCommand("cat /media/fat/downloader_ajgowans_manualsdb.ini 2>/dev/null || true");
    SshResult jsonResult = ssh.runCommand("cat /media/fat/Scripts/.config/update_all/update_all.json 2>/dev/null || true");
    SshResult arcadeOrganizerResult = ssh.runCommand("cat /media/fat/Scripts/update_arcade-organizer.ini 2>/dev/null || true");

    std::string mainText = mainResult.output;
    std::string arcadeText = arcadeResult.output;
    std::string biosText = biosResult.output;
    std::string manualsText = manualsResult.output;
    std::string jsonText = jsonResult.output;
    std::string arcadeOrganizerText = arcadeOrganizerResult.output;
    std::string combinedText = mainText + "\n" + arcadeText + "\n" + biosText;

    std::vector<ConfigToggle> toggles;
    std::vector<ConfigChoice> choices;
    std::vector<ConfigAction> actions;
    std::vector<ConfigItem> items;

    auto addCategory = [&](const std::string& label) {
        ConfigItem item;
        item.type = ConfigItem::Type::Category;
        item.label = label;
        items.push_back(item);
    };

    auto addToggle = [&](const std::string& label, bool enabled, bool indent = false, const std::string& dependsOn = "") {
        ConfigToggle toggle;
        toggle.label = label;
        toggle.enabled = enabled;
        toggles.push_back(toggle);
        ConfigItem item;
        item.type = ConfigItem::Type::Toggle;
        item.index = static_cast<int>(toggles.size()) - 1;
        item.label = label;
        item.indent = indent;
        item.dependsOn = dependsOn;
        items.push_back(item);
    };

    auto addChoice = [&](const std::string& label, const std::vector<std::string>& labels, int selectedIndex, bool indent = false, const std::string& dependsOn = "") {
        ConfigChoice choice;
        choice.label = label;
        choice.labels = labels;
        choice.selected = std::max(0, std::min(selectedIndex, static_cast<int>(labels.size()) - 1));
        choices.push_back(choice);
        ConfigItem item;
        item.type = ConfigItem::Type::Choice;
        item.index = static_cast<int>(choices.size()) - 1;
        item.label = label;
        item.indent = indent;
        item.dependsOn = dependsOn;
        items.push_back(item);
    };

    auto addAction = [&](const std::string& label, bool indent = false, const std::string& dependsOn = "") {
        ConfigAction action;
        action.label = label;
        actions.push_back(action);
        ConfigItem item;
        item.type = ConfigItem::Type::Action;
        item.index = static_cast<int>(actions.size()) - 1;
        item.label = label;
        item.indent = indent;
        item.dependsOn = dependsOn;
        items.push_back(item);
    };

    int mainSource = 0;
    if (contains(combinedText, "aitorgomez.net")) mainSource = 2;
    else if (contains(combinedText, "MiSTer-DB9")) mainSource = 1;

    std::string frontierFilter = extractSectionValue(combinedText, "MiSTerOrganize/MiSTer_Frontier", "filter");
    int frontierSource = 0;
    if (frontierFilter == "pico-8") frontierSource = 1;
    else if (frontierFilter == "openbor-4086") frontierSource = 2;
    else if (frontierFilter == "openbor-7533") frontierSource = 3;
    else if (frontierFilter == "openbor-4086 openbor-7533") frontierSource = 4;
    else if (frontierFilter == "pico-8 openbor-4086") frontierSource = 5;
    else if (frontierFilter == "pico-8 openbor-7533") frontierSource = 6;

    std::string rannyFilter = extractSectionValue(combinedText, "Ranny-Snice/Ranny-Snice-Wallpapers", "filter");
    int rannySource = 2;
    if (rannyFilter == "ar16-9") rannySource = 0;
    else if (rannyFilter == "ar4-3") rannySource = 1;

    const std::vector<std::pair<std::string, std::string>> manualsSources = {
        {"3do", "3DO"},
        {"arcadia2001", "Arcadia 2001"},
        {"atari2600", "Atari 2600"},
        {"atari5200", "Atari 5200"},
        {"atari7800", "Atari 7800"},
        {"atarilynx", "Atari Lynx"},
        {"atarixegs", "Atari XEGS"},
        {"avision", "Adventure Vision"},
        {"ballyastrocade", "Bally Astrocade"},
        {"bbcbridge", "BBC Bridge"},
        {"cdi", "CD-i"},
        {"channelf", "Channel F"},
        {"colecovision", "ColecoVision"},
        {"creativision", "CreatiVision"},
        {"fds", "Famicom Disk System"},
        {"gameandwatch", "Game & Watch"},
        {"gameboy", "Game Boy"},
        {"gamegear", "Game Gear"},
        {"gba", "Game Boy Advance"},
        {"gbc", "Game Boy Color"},
        {"intellivision", "Intellivision"},
        {"jaguar", "Jaguar"},
        {"jaguarcd", "Jaguar CD"},
        {"lcdhandhelds", "LCD Handhelds"},
        {"megadrive", "Mega Drive"},
        {"n64", "Nintendo 64"},
        {"neogeoaes", "Neo Geo AES"},
        {"neogeocd", "Neo Geo CD"},
        {"nes", "NES"},
        {"ngp", "Neo Geo Pocket"},
        {"ngpc", "Neo Geo Pocket Color"},
        {"odyssey2", "Odyssey 2"},
        {"pokemonmini", "Pokémon Mini"},
        {"psx", "PlayStation"},
        {"pyuutajr", "Pyuuta Jr."},
        {"sega32x", "Sega 32X"},
        {"segacd", "Sega CD"},
        {"segasaturn", "Sega Saturn"},
        {"segasg1000", "SG-1000"},
        {"sms", "Master System"},
        {"snes", "SNES"},
        {"supervision", "Supervision"},
        {"turbografx16", "TurboGrafx-16"},
        {"turbografxcd", "TurboGrafx-CD"},
        {"vc4000", "VC 4000"},
        {"vectrex", "Vectrex"},
        {"wonderswanc", "WonderSwan Color"},
    };

    std::vector<std::string> manualsSelected;
    for (const auto& source : manualsSources) {
        if (sectionEnabledInText(manualsText, "ajgowans/manualsdb-" + source.first)) {
            manualsSelected.push_back(source.first);
        }
    }

    addCategory("Main Cores");
    addToggle("Enable Main Cores", sectionEnabledInText(combinedText, "distribution_mister"));
    addChoice("Source", {"MiSTer-devel", "DB9 / SNAC8", "AitorGomez"}, mainSource, true, "Enable Main Cores");

    addCategory("JTCores");
    addToggle("Enable JTCores", sectionEnabledInText(combinedText, "jtcores"));
    addToggle("Enable Beta Cores", jsonBoolValue(jsonText, "download_beta_cores"), true, "Enable JTCores");

    addCategory("Other Cores");
    addToggle("Coin-Op Collection", sectionEnabledInText(combinedText, "Coin-OpCollection/Distribution-MiSTerFPGA"));
    addToggle("Arcade Offset Folder", sectionEnabledInText(combinedText, "arcade_offset_folder"));
    addToggle("LLAPI Forks Folder", sectionEnabledInText(combinedText, "llapi_folder"));
    addToggle("Unofficial Distribution", sectionEnabledInText(combinedText, "theypsilon_unofficial_distribution"));
    addToggle("Y/C Builds", sectionEnabledInText(combinedText, "MikeS11/YC_Builds-MiSTer"));
    addToggle("agg23 MiSTer Cores", sectionEnabledInText(combinedText, "agg23_db"));
    addToggle("Alt Cores", sectionEnabledInText(combinedText, "ajgowans/alt-cores"));
    addToggle("Dual RAM Console Cores", sectionEnabledInText(combinedText, "TheJesusFish/Dual-Ram-Console-Cores"));
    addToggle("MiSTer Frontier", sectionEnabledInText(combinedText, "MiSTerOrganize/MiSTer_Frontier"));
    addChoice("Filter", {"All Frontier", "PICO-8", "OpenBOR 4086", "OpenBOR 7533", "OpenBOR 4086 + 7533", "PICO-8 + OpenBOR 4086", "PICO-8 + OpenBOR 7533"}, frontierSource, true, "MiSTer Frontier");

    addCategory("Tools & Scripts");
    addToggle("Arcade Organizer", iniBoolValue(arcadeOrganizerText, "ARCADE_ORGANIZER") || jsonBoolValue(jsonText, "introduced_arcade_names_txt"));
    addToggle("MiSTer Extensions", sectionEnabledInText(combinedText, "mrext/all"));
    addToggle("MiSTer SAM", sectionEnabledInText(combinedText, "MiSTer_SAM_files"));
    addToggle("tty2oled Add-on", sectionEnabledInText(combinedText, "tty2oled_files"));
    addToggle("i2c2oled Add-on", sectionEnabledInText(combinedText, "i2c2oled_files"));
    addToggle("RetroSpy Utility", sectionEnabledInText(combinedText, "retrospy/retrospy-MiSTer"));
    addToggle("Anime0t4ku Scripts", sectionEnabledInText(combinedText, "anime0t4ku_mister_scripts"));

    addCategory("Extra Content");
    addToggle("BIOS Database", sectionEnabledInText(combinedText, "bios_db"));
    addToggle("Arcade ROMs Database", sectionEnabledInText(combinedText, "arcade_roms_db"));
    addToggle("Uberyoji Boot ROMs", sectionEnabledInText(combinedText, "uberyoji_mister_boot_roms_mgl"));
    addToggle("Dinierto GBA Borders", sectionEnabledInText(combinedText, "Dinierto/MiSTer-GBA-Borders"));
    addToggle("Anime0t4ku Wallpapers", sectionEnabledInText(combinedText, "anime0t4ku_wallpapers"));
    addToggle("PCN Challenge Wallpapers", sectionEnabledInText(combinedText, "pcn_challenge_wallpapers"));
    addToggle("Ranny Snice Wallpapers", sectionEnabledInText(combinedText, "Ranny-Snice/Ranny-Snice-Wallpapers"));
    addChoice("Source", {"16:9 Wallpapers", "4:3 Wallpapers", "All Wallpapers"}, rannySource, true, "Ranny Snice Wallpapers");
    addToggle("Game Manuals (EN) DB's", !trim(manualsText).empty());
    addAction("Configure", true, "Game Manuals (EN) DB's");

    addCategory("Community Sources");
    addToggle("Insert-Coin", sectionEnabledInText(combinedText, "funkycochise/Insert-Coin"));
    addToggle("PCN Premium Member Wallpapers", sectionEnabledInText(combinedText, "pcn_premium_wallpapers"));

    int configSelected = 0;
    int configScroll = 0;
    bool shouldSave = false;
    PadState pad;
    padInitializeDefault(&pad);

    auto toggleEnabledByLabel = [&](const std::string& label) -> bool {
        for (const ConfigToggle& toggle : toggles) {
            if (toggle.label == label) return toggle.enabled;
        }
        return false;
    };

    auto itemEnabled = [&](const ConfigItem& item) -> bool {
        if (item.type == ConfigItem::Type::Category) return false;
        if (!item.dependsOn.empty()) return toggleEnabledByLabel(item.dependsOn);
        return true;
    };

    auto itemSelectable = [&](int index) -> bool {
        return index >= 0 && index < static_cast<int>(items.size()) && items[index].type != ConfigItem::Type::Category;
    };

    auto moveConfigSelection = [&](int delta) {
        if (items.empty()) return;
        int guard = 0;
        do {
            configSelected = (configSelected + delta + static_cast<int>(items.size())) % static_cast<int>(items.size());
            guard++;
        } while (!itemSelectable(configSelected) && guard < static_cast<int>(items.size()));
    };

    if (!itemSelectable(configSelected)) moveConfigSelection(1);

    auto drawManualsMenu = [&]() {
        std::vector<std::string> working = manualsSelected;
        int manualSelected = 0;
        int manualScroll = 0;
        bool saveManuals = false;
        PadState manualsPad;
        padInitializeDefault(&manualsPad);

        while (appletMainLoop()) {
            padUpdate(&manualsPad);
            u64 held = padGetButtons(&manualsPad);
            if (!(held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X | HidNpadButton_Y | HidNpadButton_Plus | HidNpadButton_Up | HidNpadButton_Down))) break;
        }

        auto manualIsSelected = [&](const std::string& id) -> bool {
            return std::find(working.begin(), working.end(), id) != working.end();
        };


        auto drawManuals = [&]() {
            ui.beginFrame();
            ui.clear(UiRenderer::rgb(12, 10, 20));
            ui.fillRect(0, 0, UiRenderer::Width, 96, UiRenderer::rgb(20, 16, 34));
            ui.fillRect(0, 94, UiRenderer::Width, 4, UiRenderer::rgb(143, 84, 255));
            ui.drawText(42, 32, "GAME MANUALS DB'S", UiRenderer::rgb(248, 245, 255), 3);
            ui.drawCard(40, 124, 1200, 500, "MANUAL DATABASES");

            const int visible = 10;
            if (manualSelected < manualScroll) manualScroll = manualSelected;
            if (manualSelected >= manualScroll + visible) manualScroll = manualSelected - visible + 1;
            manualScroll = std::max(0, std::min(manualScroll, std::max(0, static_cast<int>(manualsSources.size()) - visible)));

            int y = 188;
            for (int row = 0; row < visible; row++) {
                int index = manualScroll + row;
                if (index >= static_cast<int>(manualsSources.size())) break;
                bool active = index == manualSelected;
                if (active) {
                    ui.fillRect(70, y - 10, 1140, 38, UiRenderer::rgb(124, 70, 220));
                    ui.drawRect(70, y - 10, 1140, 38, UiRenderer::rgb(184, 140, 255), 2);
                }
                const auto& source = manualsSources[index];
                ui.drawText(90, y, std::string(manualIsSelected(source.first) ? "[X] " : "[ ] ") + source.second, UiRenderer::rgb(248, 245, 255), 2);
                y += 42;
            }

            ui.drawText(76, 638, std::to_string(manualSelected + 1) + " / " + std::to_string(manualsSources.size()), UiRenderer::rgb(174, 154, 218), 2);
            ui.drawFooter("A TOGGLE    X SAVE & BACK    B CANCEL");
            ui.endFrame();
        };

        while (appletMainLoop()) {
            drawManuals();
            padUpdate(&manualsPad);
            u64 down = padGetButtonsDown(&manualsPad);
            if (down & HidNpadButton_Up) manualSelected = (manualSelected + static_cast<int>(manualsSources.size()) - 1) % static_cast<int>(manualsSources.size());
            if (down & HidNpadButton_Down) manualSelected = (manualSelected + 1) % static_cast<int>(manualsSources.size());
            if (down & HidNpadButton_A) {
                const std::string& id = manualsSources[manualSelected].first;
                auto found = std::find(working.begin(), working.end(), id);
                if (found == working.end()) working.push_back(id);
                else working.erase(found);
            }
            if (down & HidNpadButton_X) {
                saveManuals = true;
                break;
            }
            if (down & HidNpadButton_B) {
                saveManuals = false;
                break;
            }
        }

        if (saveManuals) manualsSelected = working;

        while (appletMainLoop()) {
            padUpdate(&manualsPad);
            u64 held = padGetButtons(&manualsPad);
            if (!(held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X | HidNpadButton_Y | HidNpadButton_Plus))) break;
        }
    };

    auto drawConfig = [&]() {
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, 96, UiRenderer::rgb(20, 16, 34));
        ui.fillRect(0, 94, UiRenderer::Width, 4, UiRenderer::rgb(143, 84, 255));
        ui.drawText(42, 32, "UPDATE ALL CONFIG", UiRenderer::rgb(248, 245, 255), 3);
        ui.drawCard(40, 124, 1200, 500, "UPDATE_ALL SOURCES");

        const int visible = 10;
        if (configSelected < configScroll) configScroll = configSelected;
        if (configSelected >= configScroll + visible) configScroll = configSelected - visible + 1;
        configScroll = std::max(0, std::min(configScroll, std::max(0, static_cast<int>(items.size()) - visible)));

        int y = 188;
        for (int row = 0; row < visible; row++) {
            int index = configScroll + row;
            if (index >= static_cast<int>(items.size())) break;
            const ConfigItem& item = items[index];
            bool active = index == configSelected;
            bool enabled = itemEnabled(item);

            if (active) {
                ui.fillRect(70, y - 10, 1140, 38, UiRenderer::rgb(124, 70, 220));
                ui.drawRect(70, y - 10, 1140, 38, UiRenderer::rgb(184, 140, 255), 2);
            }

            if (item.type == ConfigItem::Type::Category) {
                ui.drawText(90, y, item.label, UiRenderer::rgb(184, 140, 255), 2);
                ui.fillRect(90, y + 25, 1080, 2, UiRenderer::rgb(70, 52, 105));
            } else if (item.type == ConfigItem::Type::Toggle) {
                const ConfigToggle& toggle = toggles[item.index];
                const u32 textColor = enabled ? UiRenderer::rgb(248, 245, 255) : UiRenderer::rgb(120, 112, 140);
                const u32 valueColor = enabled
                    ? (toggle.enabled ? UiRenderer::rgb(112, 232, 165) : UiRenderer::rgb(218, 208, 238))
                    : UiRenderer::rgb(120, 112, 140);
                const int x = item.indent ? 130 : 90;
                ui.drawText(x, y, std::string(toggle.enabled ? "[X] " : "[ ] ") + toggle.label, textColor, 2);
                ui.drawText(1010, y, toggle.enabled ? "ON" : "OFF", valueColor, 2);
            } else if (item.type == ConfigItem::Type::Choice) {
                const ConfigChoice& choice = choices[item.index];
                const u32 textColor = enabled ? UiRenderer::rgb(248, 245, 255) : UiRenderer::rgb(120, 112, 140);
                const u32 valueColor = enabled ? UiRenderer::rgb(218, 208, 238) : UiRenderer::rgb(120, 112, 140);
                const int x = item.indent ? 130 : 90;
                ui.drawText(x, y, item.label + ":", textColor, 2);
                ui.drawText(720, y, choice.labels[choice.selected], valueColor, 2);
            } else if (item.type == ConfigItem::Type::Action) {
                const u32 textColor = enabled ? UiRenderer::rgb(248, 245, 255) : UiRenderer::rgb(120, 112, 140);
                const int x = item.indent ? 130 : 90;
                ui.drawText(x, y, "> " + actions[item.index].label, textColor, 2);
                if (enabled) ui.drawText(1010, y, "OPEN", UiRenderer::rgb(218, 208, 238), 2);
            }
            y += 42;
        }

        ui.drawText(76, 638, std::to_string(configSelected + 1) + " / " + std::to_string(items.size()), UiRenderer::rgb(174, 154, 218), 2);
        ui.drawFooter("A TOGGLE/CYCLE/OPEN    X SAVE & BACK    B CANCEL");
        ui.endFrame();
    };

    while (appletMainLoop()) {
        drawConfig();
        padUpdate(&pad);
        u64 down = padGetButtonsDown(&pad);
        if (down & HidNpadButton_Up) moveConfigSelection(-1);
        if (down & HidNpadButton_Down) moveConfigSelection(1);
        if (down & HidNpadButton_A) {
            ConfigItem& item = items[configSelected];
            if (!itemEnabled(item)) {
                continue;
            }
            if (item.type == ConfigItem::Type::Toggle) {
                toggles[item.index].enabled = !toggles[item.index].enabled;
            } else if (item.type == ConfigItem::Type::Choice) {
                choices[item.index].selected = (choices[item.index].selected + 1) % static_cast<int>(choices[item.index].labels.size());
            } else if (item.type == ConfigItem::Type::Action && actions[item.index].label == "Configure") {
                drawManualsMenu();
            }
        }
        if (down & HidNpadButton_X) {
            shouldSave = true;
            break;
        }
        if (down & HidNpadButton_B) {
            shouldSave = false;
            break;
        }
    }

    if (!shouldSave) {
        lastMessage = "Update All config cancelled.";
        return;
    }

    std::vector<std::string> mainLines = splitLines(mainText);
    std::vector<std::string> arcadeLines = splitLines(arcadeText);
    std::vector<std::string> biosLines = splitLines(biosText);

    const std::vector<std::string> mainKnownSections = {
        "distribution_mister", "jtcores", "Coin-OpCollection/Distribution-MiSTerFPGA", "arcade_offset_folder", "llapi_folder",
        "theypsilon_unofficial_distribution", "MikeS11/YC_Builds-MiSTer", "agg23_db", "ajgowans/alt-cores", "TheJesusFish/Dual-Ram-Console-Cores",
        "MiSTerOrganize/MiSTer_Frontier", "mrext/all", "MiSTer_SAM_files", "tty2oled_files", "i2c2oled_files", "retrospy/retrospy-MiSTer",
        "anime0t4ku_mister_scripts", "bios_db", "arcade_roms_db", "uberyoji_mister_boot_roms_mgl", "Dinierto/MiSTer-GBA-Borders",
        "funkycochise/Insert-Coin", "anime0t4ku_wallpapers", "pcn_challenge_wallpapers", "pcn_premium_wallpapers", "Ranny-Snice/Ranny-Snice-Wallpapers"
    };
    for (const std::string& section : mainKnownSections) mainLines = removeSectionFromLines(mainLines, section);
    arcadeLines = removeSectionFromLines(arcadeLines, "arcade_roms_db");
    biosLines = removeSectionFromLines(biosLines, "bios_db");

    auto toggle = [&](int index) -> bool { return toggles[index].enabled; };
    auto choice = [&](int index) -> int { return choices[index].selected; };

    int t = 0;
    int c = 0;
    bool mainCores = toggle(t++);
    int mainChoice = choice(c++);
    bool jtcores = toggle(t++);
    bool jtBeta = toggle(t++);
    bool coinop = toggle(t++);
    bool arcadeOffset = toggle(t++);
    bool llapi = toggle(t++);
    bool unofficial = toggle(t++);
    bool yc = toggle(t++);
    bool agg23 = toggle(t++);
    bool altcores = toggle(t++);
    bool dualram = toggle(t++);
    bool frontier = toggle(t++);
    int frontierChoice = choice(c++);
    bool arcadeOrg = toggle(t++);
    bool mrext = toggle(t++);
    bool sam = toggle(t++);
    bool tty2oled = toggle(t++);
    bool i2c2oled = toggle(t++);
    bool retrospy = toggle(t++);
    bool animeScripts = toggle(t++);
    bool bios = toggle(t++);
    bool arcadeRoms = toggle(t++);
    bool bootroms = toggle(t++);
    bool gbaBorders = toggle(t++);
    bool animeWallpapers = toggle(t++);
    bool pcnChallenge = toggle(t++);
    bool rannyWallpapers = toggle(t++);
    int rannyChoice = choice(c++);
    bool manualsDb = toggle(t++);
    bool insertCoin = toggle(t++);
    bool pcnPremium = toggle(t++);

    if (mainCores) {
        std::string url = "https://raw.githubusercontent.com/MiSTer-devel/Distribution_MiSTer/main/db.json.zip";
        if (mainChoice == 1) url = "https://raw.githubusercontent.com/MiSTer-DB9/Distribution_MiSTer/main/dbencc.json.zip";
        else if (mainChoice == 2) url = "https://www.aitorgomez.net/static/mistermain/db.json.zip";
        appendSection(mainLines, {"[distribution_mister]", "db_url = " + url});
    }
    if (jtcores) appendSection(mainLines, {"[jtcores]", "db_url = https://raw.githubusercontent.com/jotego/jtcores_mister/main/jtbindb.json.zip", "filter = [MiSTer]"});
    if (coinop) appendSection(mainLines, {"[Coin-OpCollection/Distribution-MiSTerFPGA]", "db_url = https://raw.githubusercontent.com/Coin-OpCollection/Distribution-MiSTerFPGA/db/db.json.zip"});
    if (arcadeOffset) appendSection(mainLines, {"[arcade_offset_folder]", "db_url = https://raw.githubusercontent.com/Toryalai1/Arcade_Offset/db/arcadeoffsetdb.json.zip"});
    if (llapi) appendSection(mainLines, {"[llapi_folder]", "db_url = https://raw.githubusercontent.com/MiSTer-LLAPI/LLAPI_folder_MiSTer/main/llapidb.json.zip"});
    if (unofficial) appendSection(mainLines, {"[theypsilon_unofficial_distribution]", "db_url = https://raw.githubusercontent.com/theypsilon/Distribution_Unofficial_MiSTer/main/unofficialdb.json.zip"});
    if (yc) appendSection(mainLines, {"[MikeS11/YC_Builds-MiSTer]", "db_url = https://raw.githubusercontent.com/MikeS11/YC_Builds-MiSTer/db/db.json.zip"});
    if (agg23) appendSection(mainLines, {"[agg23_db]", "db_url = https://raw.githubusercontent.com/agg23/mister-repository/db/manifest.json"});
    if (altcores) appendSection(mainLines, {"[ajgowans/alt-cores]", "db_url = https://raw.githubusercontent.com/ajgowans/alt-cores/db/db.json.zip"});
    if (dualram) appendSection(mainLines, {"[TheJesusFish/Dual-Ram-Console-Cores]", "db_url = https://raw.githubusercontent.com/TheJesusFish/Dual-Ram-Console-Cores/db/db.json.zip"});
    if (frontier) {
        std::vector<std::string> lines = {"[MiSTerOrganize/MiSTer_Frontier]", "db_url = https://raw.githubusercontent.com/MiSTerOrganize/MiSTer_Frontier/db/db.json.zip"};
        const std::vector<std::string> filters = {"", "pico-8", "openbor-4086", "openbor-7533", "openbor-4086 openbor-7533", "pico-8 openbor-4086", "pico-8 openbor-7533"};
        if (!filters[frontierChoice].empty()) lines.push_back("filter = " + filters[frontierChoice]);
        appendSection(mainLines, lines);
    }
    if (mrext) appendSection(mainLines, {"[mrext/all]", "db_url = https://raw.githubusercontent.com/wizzomafizzo/mrext/main/releases/all.json"});
    if (sam) appendSection(mainLines, {"[MiSTer_SAM_files]", "db_url = https://raw.githubusercontent.com/mrchrisster/MiSTer_SAM/db/db.json.zip"});
    if (tty2oled) appendSection(mainLines, {"[tty2oled_files]", "db_url = https://raw.githubusercontent.com/venice1200/MiSTer_tty2oled/main/tty2oleddb.json"});
    if (i2c2oled) appendSection(mainLines, {"[i2c2oled_files]", "db_url = https://raw.githubusercontent.com/venice1200/MiSTer_i2c2oled/main/i2c2oleddb.json"});
    if (retrospy) appendSection(mainLines, {"[retrospy/retrospy-MiSTer]", "db_url = https://raw.githubusercontent.com/retrospy/retrospy-MiSTer/db/db.json.zip"});
    if (animeScripts) appendSection(mainLines, {"[anime0t4ku_mister_scripts]", "db_url = https://raw.githubusercontent.com/Anime0t4ku/0t4ku-mister-scripts/db/db/scripts.json.zip"});
    if (bios) appendSection(biosLines, {"[bios_db]", "db_url = https://raw.githubusercontent.com/ajgowans/BiosDB_MiSTer/db/bios_db.json.zip"});
    if (arcadeRoms) appendSection(arcadeLines, {"[arcade_roms_db]", "db_url = https://raw.githubusercontent.com/zakk4223/ArcadeROMsDB_MiSTer/db/arcade_roms_db.json.zip"});
    if (bootroms) appendSection(mainLines, {"[uberyoji_mister_boot_roms_mgl]", "db_url = https://raw.githubusercontent.com/uberyoji/mister-boot-roms/main/db/uberyoji_mister_boot_roms_mgl.json"});
    if (gbaBorders) appendSection(mainLines, {"[Dinierto/MiSTer-GBA-Borders]", "db_url = https://raw.githubusercontent.com/Dinierto/MiSTer-GBA-Borders/db/db.json.zip"});
    if (insertCoin) appendSection(mainLines, {"[funkycochise/Insert-Coin]", "db_url = https://raw.githubusercontent.com/funkycochise/Insert-Coin/db/db.json.zip"});
    if (animeWallpapers) appendSection(mainLines, {"[anime0t4ku_wallpapers]", "db_url = https://raw.githubusercontent.com/Anime0t4ku/MiSTerWallpapers/db/db/0t4kuwallpapers.json.zip"});
    if (pcnChallenge) appendSection(mainLines, {"[pcn_challenge_wallpapers]", "db_url = https://raw.githubusercontent.com/Anime0t4ku/MiSTerWallpapers/db/db/pcnchallenge.json.zip"});
    if (pcnPremium) appendSection(mainLines, {"[pcn_premium_wallpapers]", "db_url = https://raw.githubusercontent.com/Anime0t4ku/MiSTerWallpapers/db/db/pcnpremium.json.zip"});
    if (rannyWallpapers) {
        std::string filter = rannyChoice == 0 ? "ar16-9" : (rannyChoice == 1 ? "ar4-3" : "all");
        appendSection(mainLines, {"[Ranny-Snice/Ranny-Snice-Wallpapers]", "db_url = https://raw.githubusercontent.com/Ranny-Snice/Ranny-Snice-Wallpapers/db/db.json.zip", "filter = " + filter});
    }

    std::string manualsDbText;
    if (manualsDb && !manualsSelected.empty()) {
        for (size_t i = 0; i < manualsSelected.size(); i++) {
            if (i) manualsDbText += "\n";
            manualsDbText += "[ajgowans/manualsdb-" + manualsSelected[i] + "]\n";
            manualsDbText += "db_url = https://raw.githubusercontent.com/ajgowans/manualsdb-" + manualsSelected[i] + "/db/db.json.zip\n";
        }
    }

    std::string newJson = trim(jsonText).empty()
        ? "{\n    \"migration_version\": 6,\n    \"theme\": \"Blue Installer\",\n    \"mirror\": \"\",\n    \"countdown_time\": 15,\n    \"log_viewer\": true,\n    \"use_settings_screen_theme_in_log_viewer\": true,\n    \"autoreboot\": true,\n    \"download_beta_cores\": false,\n    \"names_region\": \"JP\",\n    \"names_char_code\": \"CHAR18\",\n    \"names_sort_code\": \"Common\",\n    \"introduced_arcade_names_txt\": true,\n    \"pocket_firmware_update\": false,\n    \"pocket_backup\": false,\n    \"timeline_after_logs\": true,\n    \"overscan\": \"medium\",\n    \"monochrome_ui\": false\n}\n"
        : jsonText;
    newJson = setJsonBool(newJson, "download_beta_cores", jtBeta);
    newJson = setJsonBool(newJson, "introduced_arcade_names_txt", arcadeOrg);

    mainLines = normalizeIniLines(mainLines);
    arcadeLines = normalizeIniLines(arcadeLines);
    biosLines = normalizeIniLines(biosLines);

    std::string command;
    command += writeTextCommand(mainPath, joinLines(mainLines));
    command += "\n";
    command += writeTextCommand(arcadePath, joinLines(arcadeLines));
    command += "\n";
    command += writeTextCommand(biosPath, joinLines(biosLines));
    command += "\n";
    command += writeTextCommand(jsonPath, newJson);
    command += "\n";
    if (arcadeOrg) {
        command += writeTextCommand(arcadeOrganizerPath, "ARCADE_ORGANIZER=true\nSKIPALTS=false\n");
    } else {
        command += "rm -f " + shellQuote(arcadeOrganizerPath) + "\n";
    }
    if (manualsDb && !manualsDbText.empty()) {
        command += writeTextCommand(manualsPath, manualsDbText);
    } else {
        command += "rm -f " + shellQuote(manualsPath) + "\n";
    }

    ui.beginFrame();
    ui.clear(UiRenderer::rgb(12, 10, 20));
    ui.drawCard(260, 240, 760, 180, "SAVING CONFIG");
    ui.drawText(310, 330, "PLEASE WAIT WHILE CONFIG IS SAVED", UiRenderer::rgb(248, 245, 255), 2);
    ui.endFrame();

    SshResult saveResult = ssh.runCommand(command);
    if (saveResult.success) {
        lastMessage = "Update All config saved.";
    } else {
        lastMessage = saveResult.error.empty() ? "Update All config save failed." : saveResult.error;
    }
}

void App::configureCifsMount() {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }

    auto cifsValue = [](const std::string& text, const std::string& key) -> std::string {
        for (const std::string& rawLine : splitLines(text)) {
            std::string line = trim(rawLine);
            if (line.empty() || line.front() == '#' || line.front() == ';') continue;
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            if (trim(line.substr(0, eq)) != key) continue;
            std::string value = trim(line.substr(eq + 1));
            if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }
            return value;
        }
        return "";
    };

    SshResult existingResult = ssh.runCommand(std::string("cat ") + CifsConfigPath + " 2>/dev/null || true");
    const std::string existing = existingResult.success ? existingResult.output : "";

    std::string server = cifsValue(existing, "SERVER");
    std::string share = cifsValue(existing, "SHARE");
    std::string username = cifsValue(existing, "USERNAME");
    std::string password = cifsValue(existing, "PASSWORD");
    std::string mountValue = toLower(cifsValue(existing, "MOUNT_AT_BOOT"));
    bool mountAtBoot = mountValue.empty() ? true : mountValue != "false";

    std::string screenMessage = "";
    int field = 0;
    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 held = padGetButtons(&pad);
        if ((held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X)) == 0) break;
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.drawCard(260, 240, 760, 180, "CIFS CONFIG");
        ui.drawText(320, 330, "PREPARING CONFIGURATION SCREEN", UiRenderer::rgb(248, 245, 255), 2);
        ui.endFrame();
    }

    auto maskedPassword = [&]() -> std::string {
        if (password.empty()) return "Not set";
        return std::string(password.size() > 16 ? 16 : password.size(), '*');
    };

    while (appletMainLoop()) {
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, UiRenderer::Height, UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, 96, UiRenderer::rgb(20, 16, 34));
        ui.fillRect(0, 94, UiRenderer::Width, 4, UiRenderer::rgb(143, 84, 255));
        ui.drawText(42, 32, "CIFS MOUNT CONFIG", UiRenderer::rgb(248, 245, 255), 3);
        ui.drawStatusPill(UiRenderer::Width - 250, 28, "CONFIG", true);

        ui.drawCard(80, 138, 1120, 470, "NETWORK SHARE");
        ui.drawText(118, 204, "SERVER IP", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 186, 700, 46, safeText(server), field == 0, false, false);

        ui.drawText(118, 268, "SHARE NAME", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 250, 700, 46, safeText(share), field == 1, false, false);

        ui.drawText(118, 332, "USERNAME", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 314, 700, 46, safeText(username, "Optional"), field == 2, false, false);

        ui.drawText(118, 396, "PASSWORD", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 378, 700, 46, maskedPassword(), field == 3, false, false);

        ui.drawText(118, 460, "MOUNT AT BOOT", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 442, 700, 46, mountAtBoot ? "YES" : "NO", field == 4, false, false);

        ui.drawButton(420, 520, 700, 54, "TEST CONNECTION", field == 5, false, false);

        if (!screenMessage.empty()) {
            ui.drawMessage(screenMessage);
        }
        ui.drawFooter("A EDIT / TOGGLE / TEST    X SAVE & BACK    B CANCEL");
        ui.endFrame();

        padUpdate(&pad);
        const u64 down = padGetButtonsDown(&pad);
        if (down & HidNpadButton_Up) field = (field + 5) % 6;
        if (down & HidNpadButton_Down) field = (field + 1) % 6;

        if (down & HidNpadButton_B) {
            lastMessage = "CIFS config unchanged.";
            return;
        }

        if (down & HidNpadButton_X) {
            if (trim(server).empty()) {
                screenMessage = "Server IP is required.";
                continue;
            }
            if (trim(share).empty()) {
                screenMessage = "Share name is required.";
                continue;
            }

            std::string text =
                std::string("SERVER=\"") + server + "\"\n" +
                "SHARE=\"" + share + "\"\n" +
                "USERNAME=\"" + username + "\"\n" +
                "PASSWORD=\"" + password + "\"\n" +
                "LOCAL_DIR=\"cifs/games\"\n" +
                "WAIT_FOR_SERVER=\"true\"\n" +
                "MOUNT_AT_BOOT=\"" + std::string(mountAtBoot ? "true" : "false") + "\"\n" +
                "SINGLE_CIFS_CONNECTION=\"true\"\n";

            ui.beginFrame();
            ui.clear(UiRenderer::rgb(12, 10, 20));
            ui.drawCard(260, 240, 760, 180, "SAVING CIFS CONFIG");
            ui.drawText(310, 330, "PLEASE WAIT WHILE CONFIG IS SAVED", UiRenderer::rgb(248, 245, 255), 2);
            ui.endFrame();

            SshResult result = ssh.runCommand(std::string("mkdir -p /media/fat/Scripts && printf %s ") + shellQuote(text) + " > " + CifsConfigPath);
            if (result.success) {
                lastMessage = "CIFS config saved.";
            } else {
                lastMessage = result.error.empty() ? "CIFS config save failed." : result.error;
            }
            return;
        }

        if (down & HidNpadButton_A) {
            switch (field) {
                case 0:
                    editText("Server IP", server);
                    screenMessage.clear();
                    break;
                case 1:
                    editText("Share Name", share);
                    screenMessage.clear();
                    break;
                case 2:
                    editText("Username", username);
                    screenMessage.clear();
                    break;
                case 3:
                    editText("Password", password, true);
                    screenMessage.clear();
                    break;
                case 4:
                    mountAtBoot = !mountAtBoot;
                    screenMessage = mountAtBoot ? "Mount at boot enabled." : "Mount at boot disabled.";
                    break;
                case 5: {
                    if (trim(server).empty() || trim(share).empty()) {
                        screenMessage = "Server IP and Share Name are required.";
                        break;
                    }
                    ui.beginFrame();
                    ui.clear(UiRenderer::rgb(12, 10, 20));
                    ui.drawCard(260, 240, 760, 180, "TESTING CIFS CONNECTION");
                    ui.drawText(310, 330, "PLEASE WAIT WHILE CONNECTION IS TESTED", UiRenderer::rgb(248, 245, 255), 2);
                    ui.endFrame();

                    std::string unc = "//" + server + "/" + share;
                    std::string cmd =
                        "mkdir -p /tmp/cifs_test && "
                        "mount -t cifs " + shellQuote(unc) + " /tmp/cifs_test "
                        "-o username=" + shellQuote(username) + ",password=" + shellQuote(password) + " && "
                        "umount /tmp/cifs_test && echo SUCCESS";
                    SshResult test = ssh.runCommand(cmd);
                    screenMessage = test.success && contains(test.output, "SUCCESS")
                        ? "Connection successful."
                        : "Unable to connect to the network share.";
                    break;
                }
            }

            while (appletMainLoop()) {
                padUpdate(&pad);
                u64 held = padGetButtons(&pad);
                if ((held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X)) == 0) break;
            }
        }
    }

    lastMessage = "CIFS config unchanged.";
}

void App::configureDavBrowser() {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }

    SshResult existingResult = ssh.runCommand(std::string("cat ") + DavBrowserConfigPath + " 2>/dev/null || true");
    const std::string existing = existingResult.success ? existingResult.output : "";

    std::string serverUrl = simpleConfigValue(existing, "SERVER_URL");
    std::string username = simpleConfigValue(existing, "USERNAME");
    std::string password = simpleConfigValue(existing, "PASSWORD");
    std::string remotePath = simpleConfigValue(existing, "REMOTE_PATH");
    bool skipTls = simpleConfigValue(existing, "SKIP_TLS_VERIFY").empty()
        ? true
        : iniBoolValue(existing, "SKIP_TLS_VERIFY", true);

    std::string screenMessage;
    int field = 0;
    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 held = padGetButtons(&pad);
        if ((held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X)) == 0) break;
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.drawCard(260, 240, 760, 180, "DAV BROWSER CONFIG");
        ui.drawText(320, 330, "PREPARING CONFIGURATION SCREEN", UiRenderer::rgb(248, 245, 255), 2);
        ui.endFrame();
    }

    auto maskedPassword = [&]() -> std::string {
        if (password.empty()) return "Not set";
        return std::string(password.size() > 16 ? 16 : password.size(), '*');
    };

    while (appletMainLoop()) {
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, UiRenderer::Height, UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, 96, UiRenderer::rgb(20, 16, 34));
        ui.fillRect(0, 94, UiRenderer::Width, 4, UiRenderer::rgb(143, 84, 255));
        ui.drawText(42, 32, "DAV BROWSER CONFIG", UiRenderer::rgb(248, 245, 255), 3);
        ui.drawStatusPill(UiRenderer::Width - 250, 28, "CONFIG", true);

        ui.drawCard(80, 138, 1120, 470, "WEBDAV CONNECTION");
        ui.drawText(118, 204, "SERVER URL", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 186, 700, 46, safeText(serverUrl), field == 0, false, false);

        ui.drawText(118, 268, "USERNAME", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 250, 700, 46, safeText(username), field == 1, false, false);

        ui.drawText(118, 332, "PASSWORD", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 314, 700, 46, maskedPassword(), field == 2, false, false);

        ui.drawText(118, 396, "REMOTE PATH", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 378, 700, 46, safeText(remotePath, "Server root"), field == 3, false, false);

        ui.drawText(118, 460, "SKIP TLS VERIFY", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 442, 700, 46, boolText(skipTls), field == 4, false, false);

        if (!screenMessage.empty()) ui.drawMessage(screenMessage);
        ui.drawFooter("A EDIT / TOGGLE    X SAVE & BACK    B CANCEL");
        ui.endFrame();

        padUpdate(&pad);
        const u64 down = padGetButtonsDown(&pad);
        if (down & HidNpadButton_Up) field = (field + 4) % 5;
        if (down & HidNpadButton_Down) field = (field + 1) % 5;

        if (down & HidNpadButton_B) {
            lastMessage = "DAV Browser config unchanged.";
            return;
        }

        if (down & HidNpadButton_X) {
            if (trim(serverUrl).empty()) {
                screenMessage = "Server URL is required.";
                continue;
            }
            if (trim(username).empty()) {
                screenMessage = "Username is required.";
                continue;
            }
            if (password.empty()) {
                screenMessage = "Password is required.";
                continue;
            }

            std::string text =
                "SERVER_URL=" + serverUrl + "\n" +
                "USERNAME=" + username + "\n" +
                "PASSWORD=" + password + "\n" +
                "REMOTE_PATH=" + remotePath + "\n" +
                "SKIP_TLS_VERIFY=" + std::string(skipTls ? "true" : "false") + "\n";

            ui.beginFrame();
            ui.clear(UiRenderer::rgb(12, 10, 20));
            ui.drawCard(260, 240, 760, 180, "SAVING DAV CONFIG");
            ui.drawText(310, 330, "PLEASE WAIT WHILE CONFIG IS SAVED", UiRenderer::rgb(248, 245, 255), 2);
            ui.endFrame();

            SshResult result = ssh.runCommand(writeTextCommand(DavBrowserConfigPath, text));
            lastMessage = result.success ? "DAV Browser config saved." : (result.error.empty() ? "DAV Browser config save failed." : result.error);
            return;
        }

        if (down & HidNpadButton_A) {
            switch (field) {
                case 0: editText("DAV Server URL", serverUrl); screenMessage.clear(); break;
                case 1: editText("DAV Username", username); screenMessage.clear(); break;
                case 2: editText("DAV Password", password, true); screenMessage.clear(); break;
                case 3: editText("DAV Remote Path", remotePath); screenMessage.clear(); break;
                case 4: skipTls = !skipTls; screenMessage = skipTls ? "TLS verification will be skipped." : "TLS verification enabled."; break;
            }

            while (appletMainLoop()) {
                padUpdate(&pad);
                u64 held = padGetButtons(&pad);
                if ((held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X)) == 0) break;
            }
        }
    }

    lastMessage = "DAV Browser config unchanged.";
}

void App::configureFtpSaveSync() {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }

    SshResult existingResult = ssh.runCommand(std::string("cat ") + FtpSaveSyncConfigPath + " 2>/dev/null || true");
    const std::string existing = existingResult.success ? existingResult.output : "";

    std::string protocol = simpleConfigValue(existing, "PROTOCOL");
    if (protocol.empty()) protocol = "sftp";
    protocol = toLower(protocol) == "ftp" ? "ftp" : "sftp";
    std::string host = simpleConfigValue(existing, "HOST");
    std::string port = simpleConfigValue(existing, "PORT");
    if (port.empty()) port = protocol == "sftp" ? "22" : "21";
    std::string username = simpleConfigValue(existing, "USERNAME");
    std::string password = simpleConfigValue(existing, "PASSWORD");
    std::string remoteBase = simpleConfigValue(existing, "REMOTE_BASE");
    if (remoteBase.empty()) remoteBase = "/";
    std::string deviceName = simpleConfigValue(existing, "DEVICE_NAME");
    bool syncSavestates = iniBoolValue(existing, "SYNC_SAVESTATES", false);

    std::string screenMessage;
    int field = 0;
    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 held = padGetButtons(&pad);
        if ((held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X)) == 0) break;
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.drawCard(260, 240, 760, 180, "FTP SAVE SYNC CONFIG");
        ui.drawText(320, 330, "PREPARING CONFIGURATION SCREEN", UiRenderer::rgb(248, 245, 255), 2);
        ui.endFrame();
    }

    auto maskedPassword = [&]() -> std::string {
        if (password.empty()) return "Not set";
        return std::string(password.size() > 16 ? 16 : password.size(), '*');
    };

    while (appletMainLoop()) {
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, UiRenderer::Height, UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, 96, UiRenderer::rgb(20, 16, 34));
        ui.fillRect(0, 94, UiRenderer::Width, 4, UiRenderer::rgb(143, 84, 255));
        ui.drawText(42, 32, "FTP SAVE SYNC CONFIG", UiRenderer::rgb(248, 245, 255), 3);
        ui.drawStatusPill(UiRenderer::Width - 250, 28, "CONFIG", true);

        ui.drawCard(80, 118, 1120, 510, "REMOTE SYNC CONNECTION");
        const int labelX = 118;
        const int buttonX = 420;
        const int startY = 172;
        const int stepY = 54;

        ui.drawText(labelX, startY + 12, "PROTOCOL", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(buttonX, startY, 700, 42, protocol == "sftp" ? "SFTP (recommended)" : "FTP", field == 0, false, false);
        ui.drawText(labelX, startY + stepY + 12, "HOST", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(buttonX, startY + stepY, 700, 42, safeText(host), field == 1, false, false);
        ui.drawText(labelX, startY + stepY * 2 + 12, "PORT", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(buttonX, startY + stepY * 2, 700, 42, safeText(port), field == 2, false, false);
        ui.drawText(labelX, startY + stepY * 3 + 12, "USERNAME", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(buttonX, startY + stepY * 3, 700, 42, safeText(username), field == 3, false, false);
        ui.drawText(labelX, startY + stepY * 4 + 12, "PASSWORD", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(buttonX, startY + stepY * 4, 700, 42, maskedPassword(), field == 4, false, false);
        ui.drawText(labelX, startY + stepY * 5 + 12, "REMOTE BASE", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(buttonX, startY + stepY * 5, 700, 42, safeText(remoteBase, "/"), field == 5, false, false);
        ui.drawText(labelX, startY + stepY * 6 + 12, "DEVICE NAME", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(buttonX, startY + stepY * 6, 700, 42, safeText(deviceName), field == 6, false, false);
        ui.drawText(labelX, startY + stepY * 7 + 12, "SYNC SAVESTATES", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(buttonX, startY + stepY * 7, 700, 42, boolText(syncSavestates), field == 7, false, false);

        if (syncSavestates) {
            ui.drawText(buttonX, 610, "Warning: savestates may cause issues with some cores or games.", UiRenderer::rgb(255, 200, 104), 1);
        }
        if (!screenMessage.empty()) ui.drawMessage(screenMessage);
        ui.drawFooter("A EDIT / TOGGLE    X SAVE & BACK    B CANCEL");
        ui.endFrame();

        padUpdate(&pad);
        const u64 down = padGetButtonsDown(&pad);
        if (down & HidNpadButton_Up) field = (field + 7) % 8;
        if (down & HidNpadButton_Down) field = (field + 1) % 8;

        if (down & HidNpadButton_B) {
            lastMessage = "FTP Save Sync config unchanged.";
            return;
        }

        if (down & HidNpadButton_X) {
            if (trim(host).empty()) { screenMessage = "Host is required."; continue; }
            if (trim(port).empty()) { screenMessage = "Port is required."; continue; }
            bool portOk = true;
            for (char c : port) {
                if (!std::isdigit(static_cast<unsigned char>(c))) { portOk = false; break; }
            }
            if (!portOk) { screenMessage = "Port must be a number."; continue; }
            if (trim(username).empty()) { screenMessage = "Username is required."; continue; }
            if (password.empty()) { screenMessage = "Password is required."; continue; }
            if (trim(deviceName).empty()) { screenMessage = "Device Name is required."; continue; }
            if (trim(remoteBase).empty()) remoteBase = "/";
            if (!remoteBase.empty() && remoteBase.front() != '/') remoteBase = "/" + remoteBase;

            std::string text =
                "PROTOCOL=" + protocol + "\n" +
                "HOST=" + host + "\n" +
                "PORT=" + port + "\n" +
                "USERNAME=" + username + "\n" +
                "PASSWORD=" + password + "\n" +
                "REMOTE_BASE=" + remoteBase + "\n" +
                "DEVICE_NAME=" + deviceName + "\n\n" +
                "SYNC_SAVES=true\n" +
                "SYNC_SAVESTATES=" + std::string(syncSavestates ? "true" : "false") + "\n" +
                "SYNC_INTERVAL=15\n\n" +
                "SKIP_HOST_KEY_CHECK=true\n" +
                "SKIP_TLS_VERIFY=false\n" +
                "PAUSE_WHILE_CORE_RUNNING=true\n\n" +
                "MIN_AGE_SECONDS=5\n";

            ui.beginFrame();
            ui.clear(UiRenderer::rgb(12, 10, 20));
            ui.drawCard(260, 240, 760, 180, "SAVING FTP CONFIG");
            ui.drawText(310, 330, "PLEASE WAIT WHILE CONFIG IS SAVED", UiRenderer::rgb(248, 245, 255), 2);
            ui.endFrame();

            SshResult result = ssh.runCommand(writeTextCommand(FtpSaveSyncConfigPath, text));
            lastMessage = result.success ? "FTP Save Sync config saved." : (result.error.empty() ? "FTP Save Sync config save failed." : result.error);
            return;
        }

        if (down & HidNpadButton_A) {
            switch (field) {
                case 0: {
                    bool wasSftp = protocol == "sftp";
                    protocol = wasSftp ? "ftp" : "sftp";
                    if (protocol == "sftp" && (port.empty() || port == "21")) port = "22";
                    if (protocol == "ftp" && (port.empty() || port == "22")) port = "21";
                    screenMessage = protocol == "sftp" ? "Protocol set to SFTP." : "Protocol set to FTP.";
                    break;
                }
                case 1: editText("Host", host); screenMessage.clear(); break;
                case 2: editText("Port", port); screenMessage.clear(); break;
                case 3: editText("Username", username); screenMessage.clear(); break;
                case 4: editText("Password", password, true); screenMessage.clear(); break;
                case 5: editText("Remote Base", remoteBase); screenMessage.clear(); break;
                case 6: editText("Device Name", deviceName); screenMessage.clear(); break;
                case 7: syncSavestates = !syncSavestates; screenMessage = syncSavestates ? "Savestate sync enabled." : "Savestate sync disabled."; break;
            }

            while (appletMainLoop()) {
                padUpdate(&pad);
                u64 held = padGetButtons(&pad);
                if ((held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X)) == 0) break;
            }
        }
    }

    lastMessage = "FTP Save Sync config unchanged.";
}

void App::configureRaViewer() {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }

    SshResult existingResult = ssh.runCommand(std::string("cat ") + RaViewerConfigPath + " 2>/dev/null || true");
    const std::string existing = existingResult.success ? existingResult.output : "";

    std::string username = simpleConfigValue(existing, "username");
    std::string apiKey = simpleConfigValue(existing, "api_key");
    std::string screenMessage;
    int field = 0;
    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 held = padGetButtons(&pad);
        if ((held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X)) == 0) break;
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.drawCard(260, 240, 760, 180, "RA VIEWER CONFIG");
        ui.drawText(320, 330, "PREPARING CONFIGURATION SCREEN", UiRenderer::rgb(248, 245, 255), 2);
        ui.endFrame();
    }

    auto maskedApiKey = [&]() -> std::string {
        if (apiKey.empty()) return "Not set";
        return std::string(apiKey.size() > 20 ? 20 : apiKey.size(), '*');
    };

    while (appletMainLoop()) {
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, UiRenderer::Height, UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, 96, UiRenderer::rgb(20, 16, 34));
        ui.fillRect(0, 94, UiRenderer::Width, 4, UiRenderer::rgb(143, 84, 255));
        ui.drawText(42, 32, "RA VIEWER CONFIG", UiRenderer::rgb(248, 245, 255), 3);
        ui.drawStatusPill(UiRenderer::Width - 250, 28, "CONFIG", true);

        ui.drawCard(80, 170, 1120, 340, "RETROACHIEVEMENTS LOGIN");
        ui.drawText(118, 250, "USERNAME", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 232, 700, 46, safeText(username), field == 0, false, false);
        ui.drawText(118, 314, "API KEY", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 296, 700, 46, maskedApiKey(), field == 1, false, false);

        if (!screenMessage.empty()) ui.drawMessage(screenMessage);
        ui.drawFooter("A EDIT    X SAVE & BACK    B CANCEL");
        ui.endFrame();

        padUpdate(&pad);
        const u64 down = padGetButtonsDown(&pad);
        if (down & HidNpadButton_Up) field = (field + 1) % 2;
        if (down & HidNpadButton_Down) field = (field + 1) % 2;

        if (down & HidNpadButton_B) {
            lastMessage = "RA Viewer config unchanged.";
            return;
        }

        if (down & HidNpadButton_X) {
            if (trim(username).empty()) { screenMessage = "Username is required."; continue; }
            if (trim(apiKey).empty()) { screenMessage = "API Key is required."; continue; }

            std::string text =
                std::string("[retroachievements]\n") +
                "username = " + username + "\n" +
                "api_key = " + apiKey + "\n";

            ui.beginFrame();
            ui.clear(UiRenderer::rgb(12, 10, 20));
            ui.drawCard(260, 240, 760, 180, "SAVING RA VIEWER CONFIG");
            ui.drawText(300, 330, "PLEASE WAIT WHILE CONFIG IS SAVED", UiRenderer::rgb(248, 245, 255), 2);
            ui.endFrame();

            SshResult result = ssh.runCommand(writeTextCommand(RaViewerConfigPath, text));
            lastMessage = result.success ? "RA Viewer config saved." : (result.error.empty() ? "RA Viewer config save failed." : result.error);
            return;
        }

        if (down & HidNpadButton_A) {
            if (field == 0) {
                editText("RA Username", username);
            } else {
                editText("RA API Key", apiKey, true);
            }
            screenMessage.clear();

            while (appletMainLoop()) {
                padUpdate(&pad);
                u64 held = padGetButtons(&pad);
                if ((held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X)) == 0) break;
            }
        }
    }

    lastMessage = "RA Viewer config unchanged.";
}

void App::executeScriptAction(ScriptId id, int actionIndex) {
    std::vector<std::string> actions = scriptActions(id);
    if (actionIndex < 0 || actionIndex >= static_cast<int>(actions.size())) return;
    const std::string action = actions[actionIndex];

    switch (id) {
        case ScriptId::UpdateAll:
            if (action == "INSTALL") {
                const std::string command =
                    "mkdir -p /media/fat/Scripts /media/fat/Scripts/.config/update_all && "
                    "URL=https://raw.githubusercontent.com/theypsilon/Update_All_MiSTer/master/update_all.sh; "
                    "TARGET=/media/fat/Scripts/update_all.sh; "
                    "TMP=/tmp/mc_update_all.sh; "
                    "WGET_LOG=/tmp/mc_update_all_wget.log; "
                    "rm -f \"$TMP\" \"$WGET_LOG\"; "
                    "echo Downloading update_all launcher...; "
                    "if ! command -v wget >/dev/null 2>&1; then echo wget not found.; exit 1; fi; "
                    "if ! wget --no-check-certificate -O \"$TMP\" \"$URL\" >\"$WGET_LOG\" 2>&1; then "
                    "echo Unable to download update_all launcher.; "
                    "cat \"$WGET_LOG\"; "
                    "exit 1; "
                    "fi; "
                    "if [ ! -s \"$TMP\" ]; then "
                    "echo Downloaded update_all launcher is empty.; "
                    "cat \"$WGET_LOG\"; "
                    "exit 1; "
                    "fi; "
                    "mv \"$TMP\" \"$TARGET\" && "
                    "chmod +x \"$TARGET\" && "
                    "rm -f \"$WGET_LOG\" && "
                    "echo Installation complete.";

                showStreamingCommandWindow("INSTALL UPDATE ALL", command, "Update All installed.", "Update All install failed.");
                refreshCurrentScriptStatus();
            } else if (action == "CONFIGURE") {
                configureUpdateAll();
                refreshCurrentScriptStatus();
            } else if (action == "RUN") {
                if (!confirm("RUN UPDATE ALL", "ARE YOU SURE?")) return;
                showStreamingCommandWindow("UPDATE ALL", "/media/fat/Scripts/update_all.sh", "Update All complete.", "Update All failed.");
                refreshCurrentScriptStatus();
            } else if (action == "UNINSTALL") {
                scriptUninstall("UPDATE ALL", "rm -f /media/fat/Scripts/update_all.sh");
                refreshCurrentScriptStatus();
            }
            break;
        case ScriptId::Zaparoo:
            if (action == "INSTALL") {
                const std::string command =
                    "mkdir -p /media/fat/Scripts /tmp/mc_zap && "
                    "rm -rf /tmp/mc_zap/* && "
                    "if ! command -v wget >/dev/null 2>&1; then echo wget not found.; exit 1; fi; "
                    "echo Finding latest Zaparoo release...; "
                    "JSON=/tmp/mc_zap/release.json; "
                    "wget --no-check-certificate --header='User-Agent: MiSTer-Companion-NX' -O \"$JSON\" https://api.github.com/repos/ZaparooProject/zaparoo-core/releases/latest || exit 1; "
                    "URL=$(sed -n 's/.*\\\"browser_download_url\\\": *\\\"\\([^\\\"]*mister_arm[^\\\"]*\\.zip\\)\\\".*/\\1/p' \"$JSON\" | head -1); "
                    "if [ -z \"$URL\" ]; then echo Unable to find MiSTer Zaparoo release.; exit 1; fi; "
                    "echo Downloading Zaparoo package...; "
                    "wget --no-check-certificate -O /tmp/mc_zap/zaparoo.zip \"$URL\" || exit 1; "
                    "if [ ! -s /tmp/mc_zap/zaparoo.zip ]; then echo Zaparoo download is empty.; exit 1; fi; "
                    "echo Extracting Zaparoo package...; "
                    "unzip -o /tmp/mc_zap/zaparoo.zip -d /tmp/mc_zap >/dev/null || exit 1; "
                    "SCRIPT=$(find /tmp/mc_zap -name zaparoo.sh | head -1); "
                    "if [ -z \"$SCRIPT\" ]; then echo Could not find zaparoo.sh inside release ZIP.; exit 1; fi; "
                    "cp \"$SCRIPT\" /media/fat/Scripts/zaparoo.sh && "
                    "chmod +x /media/fat/Scripts/zaparoo.sh && "
                    "echo Installation complete.";
                showStreamingCommandWindow("INSTALL ZAPAROO", command, "Zaparoo installed.", "Zaparoo install failed.");
                refreshCurrentScriptStatus();
            }
            else if (action == "ENABLE START ON BOOT") runScriptCommand("ENABLE ZAPAROO", "mkdir -p /media/fat/linux; test -f /media/fat/linux/user-startup.sh || printf '#!/bin/sh\\n' > /media/fat/linux/user-startup.sh; grep -F 'mrext/zaparoo' /media/fat/linux/user-startup.sh >/dev/null 2>&1 || printf '\\n# mrext/zaparoo\\n[[ -e /media/fat/Scripts/zaparoo.sh ]] && /media/fat/Scripts/zaparoo.sh -service $1\\n' >> /media/fat/linux/user-startup.sh; chmod +x /media/fat/linux/user-startup.sh");
            else if (action == "DISABLE START ON BOOT") runScriptCommand("DISABLE ZAPAROO", "sed -i '/mrext\\/zaparoo/d;/zaparoo.sh -service/d' /media/fat/linux/user-startup.sh 2>/dev/null || true");
            else if (action == "UNINSTALL") scriptUninstall("ZAPAROO", "rm -f /media/fat/Scripts/zaparoo.sh");
            break;
        case ScriptId::MigrateSd:
            if (action == "INSTALL") scriptInstall("MIGRATE SD", MigrateSdPath, UrlMigrateSd);
            else if (action == "UNINSTALL") scriptUninstall("MIGRATE SD", "rm -f /media/fat/Scripts/migrate_sd.sh");
            break;
        case ScriptId::CifsMount:
            if (action == "INSTALL") showStreamingCommandWindow("INSTALL CIFS", downloadCommand(UrlCifsMount, CifsMountPath) + " && " + downloadCommand(UrlCifsUmount, CifsUmountPath), "CIFS installed.", "CIFS install failed.");
            else if (action == "CONFIGURE") configureCifsMount();
            else if (action == "MOUNT") runScriptCommand("MOUNT CIFS", CifsMountPath);
            else if (action == "UNMOUNT") runScriptCommand("UNMOUNT CIFS", CifsUmountPath);
            else if (action == "REMOVE CONFIG") runScriptCommand("REMOVE CIFS CONFIG", "rm -f /media/fat/Scripts/cifs_mount.ini", true);
            else if (action == "UNINSTALL") scriptUninstall("CIFS", "rm -f /media/fat/Scripts/cifs_mount.sh /media/fat/Scripts/cifs_umount.sh");
            break;
        case ScriptId::AutoTime:
            if (action == "INSTALL") scriptInstall("AUTO TIME", AutoTimePath, UrlAutoTime);
            else if (action == "UNINSTALL") scriptUninstall("AUTO TIME", "rm -f /media/fat/Scripts/auto_time.sh");
            break;
        case ScriptId::CdGameOrganizer:
            if (action == "INSTALL") scriptInstall("CD GAME ORGANIZER", CdGameOrganizerPath, UrlCdGameOrganizer);
            else if (action == "UNINSTALL") scriptUninstall("CD GAME ORGANIZER", "rm -f /media/fat/Scripts/cd_game_organizer.sh");
            break;
        case ScriptId::DavBrowser:
            if (action == "INSTALL") scriptInstall("DAV BROWSER", DavBrowserPath, UrlDavBrowser);
            else if (action == "CONFIGURE") configureDavBrowser();
            else if (action == "REMOVE CONFIG") runScriptCommand("REMOVE DAV CONFIG", "rm -f /media/fat/Scripts/.config/dav_browser/dav_browser.ini", true);
            else if (action == "UNINSTALL") scriptUninstall("DAV BROWSER", "rm -f /media/fat/Scripts/dav_browser.sh; rm -rf /media/fat/Scripts/.config/dav_browser");
            break;
        case ScriptId::FtpSaveSync:
            if (action == "INSTALL") scriptInstall("FTP SAVE SYNC", FtpSaveSyncPath, UrlFtpSaveSync);
            else if (action == "CONFIGURE") configureFtpSaveSync();
            else if (action == "ENABLE START ON BOOT") runScriptCommand("ENABLE FTP SAVE SYNC", "mkdir -p /media/fat/linux; test -f /media/fat/linux/user-startup.sh || printf '#!/bin/sh\\n' > /media/fat/linux/user-startup.sh; grep -F 'ftp_save_sync_daemon.sh' /media/fat/linux/user-startup.sh >/dev/null 2>&1 || printf '\\n# ftp_save_sync START\\n/media/fat/Scripts/.config/ftp_save_sync/ftp_save_sync_daemon.sh >/dev/null 2>&1\\n# ftp_save_sync END\\n' >> /media/fat/linux/user-startup.sh; chmod +x /media/fat/linux/user-startup.sh");
            else if (action == "DISABLE START ON BOOT") runScriptCommand("DISABLE FTP SAVE SYNC", "sed -i '/ftp_save_sync START/,/ftp_save_sync END/d' /media/fat/linux/user-startup.sh 2>/dev/null || true");
            else if (action == "REMOVE CONFIG") runScriptCommand("REMOVE FTP CONFIG", "rm -f /media/fat/Scripts/.config/ftp_save_sync/ftp_save_sync.ini", true);
            else if (action == "UNINSTALL") scriptUninstall("FTP SAVE SYNC", "sed -i '/ftp_save_sync START/,/ftp_save_sync END/d' /media/fat/linux/user-startup.sh 2>/dev/null || true; rm -f /media/fat/Scripts/ftp_save_sync.sh; rm -rf /media/fat/Scripts/.config/ftp_save_sync");
            break;
        case ScriptId::StaticWallpaper:
            if (action == "INSTALL") scriptInstall("STATIC WALLPAPER", StaticWallpaperPath, UrlStaticWallpaper);
            else if (action == "UNINSTALL") scriptUninstall("STATIC WALLPAPER", "rm -f /media/fat/Scripts/static_wallpaper.sh; rm -rf /media/fat/Scripts/.config/static_wallpaper; rm -f /media/fat/menu.jpg /media/fat/menu.png");
            break;
        case ScriptId::Syncthing:
            if (action == "INSTALL") scriptInstall("SYNCTHING", SyncthingPath, UrlSyncthingScript);
            else if (action == "ENABLE START ON BOOT") runScriptCommand("ENABLE SYNCTHING", "mkdir -p /media/fat/linux; test -f /media/fat/linux/user-startup.sh || printf '#!/bin/sh\\n' > /media/fat/linux/user-startup.sh; grep -F 'syncthing_service.sh' /media/fat/linux/user-startup.sh >/dev/null 2>&1 || printf '\\n# Start Syncthing\\n/media/fat/Scripts/.config/syncthing/syncthing_service.sh start &\\n' >> /media/fat/linux/user-startup.sh; chmod +x /media/fat/linux/user-startup.sh");
            else if (action == "DISABLE START ON BOOT") runScriptCommand("DISABLE SYNCTHING", "sed -i '/Start Syncthing/d;/syncthing_service.sh/d' /media/fat/linux/user-startup.sh 2>/dev/null || true");
            else if (action == "UNINSTALL") scriptUninstall("SYNCTHING", "rm -f /media/fat/Scripts/syncthing.sh; rm -rf /media/fat/Scripts/.config/syncthing");
            break;
        case ScriptId::RaViewer:
            if (action == "INSTALL") scriptInstall("RA VIEWER", RaViewerPath, UrlRaViewer);
            else if (action == "EDIT CONFIG") configureRaViewer();
            else if (action == "UNINSTALL") scriptUninstall("RA VIEWER", "rm -f /media/fat/Scripts/ra_viewer.sh; rm -rf /media/fat/Scripts/.config/ra_viewer");
            break;
    }
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
