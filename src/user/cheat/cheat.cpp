#include "pch.h"
#include "cheat.h"

#include "feature_manager.h"

#include "memory/mem.h"

namespace cheat
{
    enum class MethodIndex : int
    {
        Update = 0,
        LateUpdate = 1,
        FixedUpdate = 2
    };

    static void CallUpdateMethod_Hook(void* self, MethodIndex methodIndex)
    {
        CALL_ORIGINAL(CallUpdateMethod_Hook, self, methodIndex);

        switch (methodIndex)
        {
        case MethodIndex::Update:
            SAFE_EXECUTE(EventManager::onUpdate();)
            break;
        case MethodIndex::LateUpdate:
        case MethodIndex::FixedUpdate:
        default:
            break;
        }
    }

    void init()
    {
        ConfigManager::getInstance().load();

        auto& manager = FeatureManager::getInstance();

        manager.registerFeatures<
            // Add features here
        >();

        const auto fullVersion = Application::get_version()()->ToString();
        const size_t dotPos = fullVersion.rfind('.');
        const auto version = fullVersion.substr(dotPos + 1);
        LOG_INFO("Game version: {}", version.c_str());

        manager.init();
    }

    void shutdown()
    {
        auto& hookManager = HookManager::getInstance();
        hookManager.shutdown();
        LOG_INFO("Hooks shutdown successfully");
    }

    void hookMonoBehaviour(uintptr_t unity, int versionMajor)
    {
        auto signature = "";
        switch (versionMajor)
        {
        case 2017:
            signature = "48 89 5c 24 ? 57 48 83 ec ? 48 8b 41 ? 8b fa 48 8b d9 48 85 c0 74 ? 80 78";
            break;
        case 2018:
            signature = "48 89 5c 24 ? 57 48 83 ec ? 48 8b 81 ? ? ? ? 8b fa 48 8b d9 48 85 c0 74 ? 80 78";
            break;
        case 2019:
            signature = "48 89 5c 24 ? 56 48 83 ec ? 48 8b 81 ? ? ? ? 8b f2";
            break;
        case 2020:
        case 2021:
            signature = "48 89 5c 24 ? 57 48 83 ec ? 48 8b 81 ? ? ? ? 8b fa 48 8b d9 48 85 c0 74 ? 80 78";
            break;
        case 2022:
            signature = "48 89 74 24 ? 57 48 83 ec ? 8b f2 48 8b f9 e8 ? ? ? ? 84 c0";
            break;
        case 2023:
        case 6000:
            signature = "48 89 5c 24 ? 56 48 83 ec ? 8b f2 48 8b d9 e8 ? ? ? ? 84 c0 0f 85";
            break;
        default:
            LOG_WARN("Signature for MonoBehaviour::CallUpdateMethod not set for Unity version major {}", versionMajor);
            break;
        }

        if (!signature || strlen(signature) == 0) return;

        using CallUpdateMethod_t = void(*)(void*, MethodIndex);
        auto func = reinterpret_cast<CallUpdateMethod_t>(Mem::patternScan(unity, signature));
        if (!func)
        {
            LOG_ERROR("MonoBehaviour::CallUpdateMethod not found! Signature is probably incorrect.");
            return;
        }

        // LOG_DEBUG("Found MonoBehaviour::CallUpdateMethod at {:p}", reinterpret_cast<void*>(func));

        HookManager::install(func, CallUpdateMethod_Hook);
    }
}
