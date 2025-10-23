#pragma once

#include "field.h"
#include "core/hotkey/hotkey_manager.h"

namespace config
{
    class HotkeyField : public Field<int>
    {
    public:
        using HotkeyHandler = std::function<void()>;
        using Connection = Event<>::Connection;

        HotkeyField(const std::string& ownerPath, const std::string& key, int defaultVk = 0)
            : Field(ownerPath, key, defaultVk)
            , m_isCapturing(false)
        {
            HotkeyManager::getInstance().registerField(this);

            m_changeConnection = onChanged([this](const int& oldVk, const int& newVk)
            {
                HotkeyManager::getInstance().updateHotkey(this, oldVk, newVk);
            });
        }

        ~HotkeyField() override
        {
            HotkeyManager::getInstance().unregisterField(this);
        }

        Connection setHandler(HotkeyHandler handler)
        {
            return m_onTriggered.connect(std::move(handler));
        }

        template <auto MemPtr, typename Instance>
        Connection setHandler(Instance* instance)
        {
            return m_onTriggered.connect<MemPtr>(instance);
        }

        template <auto FuncPtr>
        Connection setHandler()
        {
            return m_onTriggered.connect<FuncPtr>();
        }

        void trigger()
        {
            if (get() != 0)
            {
                // LOG_DEBUG("Hotkey triggered: {} ({})", HotkeyManager::keyName(get()), getKey());
                m_onTriggered();
            }
        }

        void beginCapture()
        {
            m_isCapturing = true;
            HotkeyManager::getInstance().beginCapture(this);
            // LOG_DEBUG("Begin hotkey capture for field '{}'", getKey());
        }

        void endCapture(int capturedVk)
        {
            m_isCapturing = false;
            set(capturedVk);
            LOG_INFO("Hotkey captured: {} for field '{}'", HotkeyManager::keyName(capturedVk), getKey());
        }

        void cancelCapture()
        {
            m_isCapturing = false;
            // LOG_DEBUG("Hotkey capture cancelled for field '{}'", getKey());
        }

        bool isCapturing() const
        {
            return m_isCapturing;
        }

        void clear()
        {
            set(0);
            // LOG_DEBUG("Hotkey cleared for field '{}'", getKey());
        }

        std::string getKeyName() const
        {
            return HotkeyManager::keyName(get());
        }

        void reload() override
        {
            int oldVk = get();
            Field::reload();
            int newVk = get();

            if (oldVk != newVk)
            {
                HotkeyManager::getInstance().updateHotkey(this, oldVk, newVk);
            }
        }

        bool drawCaptureButton()
        {
            auto& hkm = HotkeyManager::getInstance();

            ImGui::PushID(this);

            bool result = false;

            constexpr float buttonWidth = 100.0f;

            if (!hkm.isCapturing())
            {
                // Display current key binding or "Not Set"
                std::string buttonLabel = get() != 0 ? getKeyName() : "Not Set";

                if (ImGui::Button(buttonLabel.c_str(), ImVec2(buttonWidth, 0)))
                {
                    beginCapture();
                    result = true;
                }

                if (ImGui::IsItemHovered())
                {
                    if (get() != 0)
                    {
                        ImGui::SetTooltip("Click to change hotkey\nRight-click to clear");
                    }
                    else
                    {
                        ImGui::SetTooltip("Click to set hotkey");
                    }
                }

                if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && get() != 0)
                {
                    clear();
                    result = true;
                }
            }
            else if (hkm.getCurrentCapture() == this)
            {
                // Capturing state - orange color to indicate active capture
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.1f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.5f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.6f, 0.3f, 1.0f));

                ImGui::Button("Press key...", ImVec2(buttonWidth, 0));

                ImGui::PopStyleColor(3);

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Press any key to bind\nESC to cancel");
                }
            }
            else
            {
                ImGui::BeginDisabled();
                ImGui::Button("Waiting...", ImVec2(buttonWidth, 0));
                ImGui::EndDisabled();
            }

            ImGui::PopID();

            return result;
        }

    private:
        bool m_isCapturing;
        Event<> m_onTriggered;
        Field::Connection m_changeConnection;
    };
}
