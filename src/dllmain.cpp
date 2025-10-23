#include "pch.h"
#include "user/main.h"
#include <filesystem>
#include <thread>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        Logger::attach();
        if (PipeManager::isUsingPipes())
        {
            Main::run();
            PipeManager::getInstance().start();
        }
        else
        {
            std::thread(Main::run).detach();
        }
        break;

    case DLL_PROCESS_DETACH:
        if (lpReserved == nullptr)
        {
            Main::shutdown();
            if (PipeManager::isUsingPipes()) PipeManager::getInstance().stop();
        }
        break;

    default:
        break;
    }

    return TRUE;
}
