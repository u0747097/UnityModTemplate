#pragma once

class PipeManager
{
public:
    static PipeManager& getInstance();
    static bool isUsingPipes();

    void start();
    void stop();

    bool isFeatureEnabled(const std::string& featureName) const;
    void setFeatureState(const std::string& featureName, bool enabled);

    // Prevent copying
    PipeManager(const PipeManager&) = delete;
    PipeManager& operator=(const PipeManager&) = delete;

    // Prevent moving
    PipeManager(PipeManager&&) = delete;
    PipeManager& operator=(PipeManager&&) = delete;

private:
    PipeManager();
    ~PipeManager();

    void runServer();
    void handleClient(HANDLE hPipe);

    std::atomic<bool> m_running{false};
    std::thread m_serverThread;
    std::unordered_map<std::string, bool> m_features;
    mutable std::mutex m_featuresMutex;

    static constexpr auto PIPE_NAME = L"\\\\.\\pipe\\pipe00";
    static constexpr auto PIPE_FLAG_NAME = "UsePipeModeFlag_";
};
