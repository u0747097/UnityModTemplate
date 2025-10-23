#pragma once

class Main
{
public:
    static void run();
    static void shutdown();

private:
    static std::pair<void*, UnityResolve::Mode> getUnityBackend();
    static bool initializeUnity();
    static std::pair<uintptr_t, int> getUnityVersionMajor();
};
