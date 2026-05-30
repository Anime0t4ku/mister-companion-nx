#pragma once

#include "config.hpp"

#include <functional>
#include <string>

struct SshResult {
    bool success = false;
    std::string output;
    std::string error;
};

class SshClient {
public:
    SshClient();
    ~SshClient();

    bool connect(const AppConfig& config, std::string& message);
    void disconnect();
    bool isConnected() const;
    SshResult runCommand(const std::string& command);
    SshResult runCommandStreaming(const std::string& command, const std::function<void(const std::string&)>& onOutput);

private:
    int socketFd = -1;
    void* session = nullptr;
    AppConfig activeConfig;
};
