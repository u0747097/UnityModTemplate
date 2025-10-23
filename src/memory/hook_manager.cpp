#include "pch.h"
#include "hook_manager.h"

HookManager& HookManager::getInstance()
{
    static HookManager instance;
    return instance;
}

HookManager::~HookManager()
{
    shutdown();
}

bool HookManager::initialize()
{
    if (m_initialized) return true;

    auto status = MH_Initialize();
    if (status != MH_OK) return false;

    m_initialized = true;
    return true;
}

void HookManager::shutdown()
{
    if (!m_initialized) return;

    for (auto& hook : m_hooks)
    {
        if (hook->target)
        {
            MH_DisableHook(hook->target);
            MH_RemoveHook(hook->target);
        }
    }

    MH_Uninitialize();
    m_hooks.clear();
    m_detourToOriginal.clear();
    m_initialized = false;
}

bool HookManager::uninstall(void* target)
{
    auto& instance = getInstance();
    if (!instance.m_initialized) return false;

    auto it = std::ranges::find_if(instance.m_hooks,
                                   [target](const std::unique_ptr<HookInfo>& hook)
                                   {
                                       return hook->target == target;
                                   });

    if (it == instance.m_hooks.end()) return false;

    auto& hook = *it;

    if (hook->enabled)
    {
        auto status = MH_DisableHook(hook->target);
        if (status != MH_OK)
        {
            LOG_ERROR("Failed to disable hook: {}", magic_enum::enum_name(status));
            return false;
        }
    }

    auto status = MH_RemoveHook(hook->target);
    if (status != MH_OK)
    {
        LOG_ERROR("Failed to remove hook: {}", magic_enum::enum_name(status));
        return false;
    }

    instance.m_detourToOriginal.erase(hook->detour);
    instance.m_hooks.erase(it);

    return true;
}

bool HookManager::enableHook(void* target)
{
    if (!initialize() && !m_initialized) return false;

    HookInfo* hook = findHook(target);
    if (!hook) return false;

    if (hook->enabled) return true;

    auto status = MH_EnableHook(hook->target);
    if (status == MH_OK)
    {
        hook->enabled = true;
        return true;
    }

    return false;
}

bool HookManager::disableHook(void* target)
{
    if (!initialize() && !m_initialized) return false;

    HookInfo* hook = findHook(target);
    if (!hook) return false;

    if (!hook->enabled) return true;

    auto status = MH_DisableHook(hook->target);
    if (status == MH_OK)
    {
        hook->enabled = false;
        return true;
    }

    return false;
}

HookManager::HookInfo* HookManager::findHook(void* target)
{
    auto it = std::ranges::find_if(m_hooks,
                                   [target](const std::unique_ptr<HookInfo>& hook)
                                   {
                                       return hook->target == target;
                                   });

    return it != m_hooks.end() ? it->get() : nullptr;
}

void* HookManager::resolveModuleFunction(const std::string& moduleName, intptr_t offset)
{
    HMODULE hModule = GetModuleHandleA(moduleName.c_str());
    if (!hModule)
    {
        // Try to load the module if it's not already loaded
        hModule = LoadLibraryA(moduleName.c_str());
        if (!hModule)
        {
#ifdef _DEBUG
            std::string debugMsg = "HookManager: Failed to load module " + moduleName + "\n";
            OutputDebugStringA(debugMsg.c_str());
#endif
            return nullptr;
        }
    }

    auto targetFunction = reinterpret_cast<void*>(reinterpret_cast<intptr_t>(hModule) + offset);

#ifdef _DEBUG
    std::string debugMsg = "HookManager: Resolved " + moduleName + " + 0x" +
        std::to_string(offset) + " to " + std::to_string(reinterpret_cast<intptr_t>(targetFunction)) + "\n";
    OutputDebugStringA(debugMsg.c_str());
#endif

    return targetFunction;
}
