#pragma once
#include "feature_base.h"

namespace cheat
{
    class FeatureManager
    {
    public:
        static FeatureManager& getInstance();

        template <typename... Features>
        void registerFeatures()
        {
            (registerFeature(std::make_unique<Features>()), ...);
        }

        void init();
        void draw();
        void reloadConfig();

        template <typename T>
        T* getFeature(const std::string& name)
        {
            auto it = m_featureMap.find(name);
            if (it != m_featureMap.end())
            {
                return dynamic_cast<T*>(it->second);
            }
            return nullptr;
        }

        void* getFeature(const std::string& name);

        std::vector<void*> getFeaturesBySection(FeatureSection section) const;

    private:
        FeatureManager() = default;

        // TODO: Use a more type safe approach for feature management
        // This is a workaround to allow polymorphic behavior without RTTI
        // Each feature will be wrapped in a FeatureWrapper that holds function pointers
        // for the methods we need to call, allowing us to avoid dynamic_casts
        struct FeatureWrapper
        {
            std::unique_ptr<void, void(*)(void*)> feature;
            std::string name;
            std::string description;
            FeatureSection section;
            const char* sectionName;
            bool allowDraw;
            config::HotkeyField& toggleKey;

            void (*init_func)(void*);
            void (*draw_func)(void*);
            bool (*isEnabled_func)(void*);
            void (*setEnabled_func)(void*, bool);
            void (*setupConfig_func)(void*);
            void (*reloadConfig_func)(void*);

            template <typename T>
            FeatureWrapper(std::unique_ptr<T> f)
                : feature(f.release(), [](void* p) { delete static_cast<T*>(p); })
                , name(static_cast<T*>(feature.get())->getName())
                , description(static_cast<T*>(feature.get())->getDescription())
                , section(static_cast<T*>(feature.get())->getSection())
                , sectionName(static_cast<T*>(feature.get())->getSectionName())
                , allowDraw(static_cast<T*>(feature.get())->isAllowDraw())
                , toggleKey(static_cast<T*>(feature.get())->getToggleKey())
                , init_func([](void* p) { static_cast<T*>(p)->init(); })
                , draw_func([](void* p) { static_cast<T*>(p)->draw(); })
                , isEnabled_func([](void* p) { return static_cast<T*>(p)->isEnabled(); })
                , setEnabled_func([](void* p, bool v) { static_cast<T*>(p)->setEnabled(v); })
                , setupConfig_func([](void* p) { static_cast<T*>(p)->setupConfig(); })
                , reloadConfig_func([](void* p) { static_cast<T*>(p)->reloadConfig(); })
            {
            }
        };

        std::vector<FeatureWrapper> m_features;
        std::unordered_map<std::string, void*> m_featureMap;

        Event<int, bool&>::Connection m_keyConnection;
        Event<>::Connection m_reloadConnection;

        template <typename T>
        void registerFeature(std::unique_ptr<T> feature)
        {
            const std::string& name = feature->getName();
            void* ptr = feature.get();

            m_features.emplace_back(std::move(feature));
            m_featureMap[name] = ptr;
        }

        void onKeyDown(int vk, bool& handled);
        void onReloadConfig();

        void drawFeature(FeatureWrapper& wrapper);

        static const char* getSectionName(FeatureSection section);
        static void helpMarker(const char* desc);
    };
}
