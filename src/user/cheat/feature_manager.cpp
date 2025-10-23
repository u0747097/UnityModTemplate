#include "pch.h"
#include "feature_manager.h"

#include "core/events/event_manager.h"
#include "core/hotkey/hotkey_manager.h"

namespace cheat
{
    FeatureManager& FeatureManager::getInstance()
    {
        static FeatureManager instance;
        return instance;
    }

    void FeatureManager::init()
    {
        LOG_INFO("Initializing {} features...", m_features.size());

        m_keyConnection = EventManager::onKeyDown.connect<&FeatureManager::onKeyDown>(this);
        m_reloadConnection = EventManager::onReloadConfig.connect<&FeatureManager::onReloadConfig>(this);

        for (auto& wrapper : m_features)
        {
            try
            {
                wrapper.setupConfig_func(wrapper.feature.get());
                wrapper.init_func(wrapper.feature.get());
                LOG_INFO("Feature '{}' initialized successfully", wrapper.name);
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("Exception during initialization of feature '{}': {}",
                          wrapper.name, e.what());
            }
        }

        LOG_INFO("FeatureManager initialization complete");
    }

    void FeatureManager::draw()
    {
        if (ImGui::BeginTabBar("FeatureTabs", ImGuiTabBarFlags_None))
        {
            for (auto sectionIdx = 0; sectionIdx < static_cast<int>(FeatureSection::Count); ++sectionIdx)
            {
                auto section = static_cast<FeatureSection>(sectionIdx);

                std::vector<FeatureWrapper*> sectionFeatures;
                for (auto& wrapper : m_features)
                {
                    if (wrapper.section == section && wrapper.allowDraw)
                    {
                        sectionFeatures.push_back(&wrapper);
                    }
                }

                if (sectionFeatures.empty()) continue;

                if (ImGui::BeginTabItem(getSectionName(section)))
                {
                    ImGui::Spacing();

                    for (auto* wrapper : sectionFeatures)
                    {
                        drawFeature(*wrapper);
                    }

                    ImGui::EndTabItem();
                }
            }

            ImGui::EndTabBar();
        }
    }

    void FeatureManager::reloadConfig()
    {
        LOG_INFO("Reloading feature configurations...");

        // Reload all feature configs
        for (auto& wrapper : m_features)
        {
            try
            {
                wrapper.reloadConfig_func(wrapper.feature.get());
                // LOG_DEBUG("Feature '{}' config reloaded", wrapper.name);
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("Failed to reload config for feature '{}': {}",
                          wrapper.name, e.what());
            }
        }

        LOG_INFO("Feature configuration reload complete");
    }

    void FeatureManager::onKeyDown(int vk, bool& handled)
    {
        // Let the HotkeyManager handle all hotkey processing
        if (HotkeyManager::getInstance().processKey(vk))
        {
            handled = true;
        }
    }

    void FeatureManager::onReloadConfig()
    {
        // Called when global config reload event is triggered
        reloadConfig();
    }

    void FeatureManager::drawFeature(FeatureWrapper& wrapper)
    {
        ImGui::PushID(wrapper.name.c_str());

        const float contentWidth = ImGui::GetContentRegionAvail().x;
        constexpr float hotkeyButtonWidth = 100.0f;
        const float hotkeyStartPos = contentWidth - hotkeyButtonWidth - 10.0f;

        // Draw feature enabled checkbox
        bool enabled = wrapper.isEnabled_func(wrapper.feature.get());
        if (ImGui::Checkbox(wrapper.name.c_str(), &enabled))
        {
            wrapper.setEnabled_func(wrapper.feature.get(), enabled);
        }

        // Draw description help marker
        if (!wrapper.description.empty())
        {
            ImGui::SameLine();
            helpMarker(wrapper.description.c_str());
        }

        ImGui::SameLine();
        float currentPos = ImGui::GetCursorPosX();
        if (currentPos < hotkeyStartPos)
        {
            ImGui::SetCursorPosX(hotkeyStartPos);
        }

        // Draw the hotkey button
        wrapper.toggleKey.drawCaptureButton();

        // Draw the content
        // ImGui::Indent();
        wrapper.draw_func(wrapper.feature.get());
        // ImGui::Unindent();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PopID();
    }

    void* FeatureManager::getFeature(const std::string& name)
    {
        auto it = m_featureMap.find(name);
        return it != m_featureMap.end() ? it->second : nullptr;
    }

    std::vector<void*> FeatureManager::getFeaturesBySection(FeatureSection section) const
    {
        std::vector<void*> result;
        for (auto& wrapper : m_features)
        {
            if (wrapper.section == section)
            {
                result.push_back(wrapper.feature.get());
            }
        }
        return result;
    }

    const char* FeatureManager::getSectionName(FeatureSection section)
    {
        switch (section)
        {
        case FeatureSection::Player:
            return "Player";
        case FeatureSection::Combat:
            return "Combat";
        case FeatureSection::Game:
            return "Game";
        case FeatureSection::Settings:
            return "Settings";
        case FeatureSection::Debug:
            return "Debug";
        case FeatureSection::Hooks:
            return "Hooks";
        case FeatureSection::Count:
        default:
            return "Unknown";
        }
    }

    void FeatureManager::helpMarker(const char* desc)
    {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
}
