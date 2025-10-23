#include "pch.h"
#include "main.h"

#include "core/rendering/renderer.h"
#include "cheat/cheat.h"

void Main::run()
{
    LOG_INFO("Starting initialization...");
    while (!FindWindowA("UnityWndClass", nullptr))
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (!initializeUnity())
        LOG_ERROR("Unable to initialize Unity! Maybe assemblies are not found?");

    if (!PipeManager::isUsingPipes())
    {
        LOG_INFO("{}", Renderer::getInstance().initialize()
                 ? "Renderer initialized!"
                 : "Failed to initialize renderer!");
    }

    const auto [unityModule, unityVersionMajor] = getUnityVersionMajor();
    if (unityModule == 0 || unityVersionMajor == -1) return;

    cheat::init();
    cheat::hookMonoBehaviour(unityModule, unityVersionMajor);
}

void Main::shutdown()
{
    LOG_INFO("Shutting down...");

    Renderer::getInstance().shutdown();
    cheat::shutdown();
    Sleep(100);
    Logger::close();
}


std::pair<void*, UnityResolve::Mode> Main::getUnityBackend()
{
    LOG_INFO("Finding Unity backend...");

    if (auto assembly = GetModuleHandleA("GameAssembly.dll"))
    {
        LOG_INFO("Found Il2Cpp backend!");
        return {assembly, UnityResolve::Mode::Il2Cpp};
    }

    std::vector<std::string> monoModules = {
        "mono-2.0-bdwgc.dll",
        "mono.dll"
    };

    for (const auto& monoModule : monoModules)
    {
        if (const auto monoHandle = GetModuleHandleA(monoModule.c_str()); monoHandle)
        {
            LOG_INFO("Found Mono backend: %s", monoModule.c_str());
            return {monoHandle, UnityResolve::Mode::Mono};
        }
    }

    LOG_ERROR("Unable to identify Unity backend!");
    return {nullptr, UnityResolve::Mode::Mono};
}

bool Main::initializeUnity()
{
    auto [module, mode] = getUnityBackend();
    if (module == nullptr)
    {
        LOG_ERROR("Unity backend not found!");
        return false;
    }

    UnityResolve::Init(module, mode);

    return true;
}

std::pair<uintptr_t, int> Main::getUnityVersionMajor()
{
    const auto unityModule = GetModuleHandleA("UnityPlayer.dll");
    if (unityModule == nullptr)
    {
        LOG_ERROR("UnityPlayer.dll not found!");
        return {0, -1};
    }

    char path[MAX_PATH];
    GetModuleFileNameA(unityModule, path, MAX_PATH);

    DWORD ver;
    DWORD verSize = GetFileVersionInfoSizeA(path, &ver);
    if (verSize == 0) return {0, -1};

    void* verData = malloc(verSize);
    GetFileVersionInfoA(path, 0, verSize, verData);
    VS_FIXEDFILEINFO* fileInfo;
    UINT size;
    VerQueryValueA(verData, "\\", reinterpret_cast<LPVOID*>(&fileInfo), &size);

    int major = HIWORD(fileInfo->dwFileVersionMS);
    LOG_INFO("Unity Major version: {}", major);
    free(verData);
    return {reinterpret_cast<uintptr_t>(unityModule), major};
}
