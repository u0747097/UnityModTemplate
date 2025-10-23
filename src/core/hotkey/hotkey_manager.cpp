#include "pch.h"
#include "hotkey_manager.h"
#include "core/config/fields/hotkey_field.h"

HotkeyManager& HotkeyManager::getInstance()
{
    static HotkeyManager inst;
    return inst;
}

void HotkeyManager::registerField(config::HotkeyField* field)
{
    if (std::ranges::find(m_fields, field) == m_fields.end())
    {
        m_fields.push_back(field);

        // Add to VK mapping if field has a key assigned
        int vk = field->get();
        if (vk != 0)
        {
            m_vkToFields[vk].push_back(field);
        }
    }
}

void HotkeyManager::unregisterField(config::HotkeyField* field)
{
    // Remove from fields list
    std::erase(m_fields, field);

    // Remove from VK mappings
    for (auto& [vk, fieldList] : m_vkToFields)
    {
        std::erase(fieldList, field);
    }

    // Clean up empty VK entries
    for (auto it = m_vkToFields.begin(); it != m_vkToFields.end();)
    {
        if (it->second.empty())
        {
            it = m_vkToFields.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Cancel capture if this field was being captured
    if (m_currentCapture == field)
    {
        m_currentCapture = nullptr;
    }
}

void HotkeyManager::updateHotkey(config::HotkeyField* field, int oldVk, int newVk)
{
    // Remove from old VK mapping
    if (oldVk != 0)
    {
        auto it = m_vkToFields.find(oldVk);
        if (it != m_vkToFields.end())
        {
            std::erase(it->second, field);
            if (it->second.empty())
            {
                m_vkToFields.erase(it);
            }
        }
    }

    // Add to new VK mapping
    if (newVk != 0)
    {
        m_vkToFields[newVk].push_back(field);
    }
}

void HotkeyManager::setCaptured(int vk)
{
    if (m_currentCapture)
    {
        m_currentCapture->endCapture(vk);
        m_currentCapture = nullptr;
    }
}

void HotkeyManager::cancelCapture()
{
    if (m_currentCapture)
    {
        m_currentCapture->cancelCapture();
        m_currentCapture = nullptr;
    }
}

bool HotkeyManager::processKey(int vk)
{
    if (isCapturing())
    {
        if (vk == VK_ESCAPE || vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON)
        {
            cancelCapture();
            return true;
        }

        setCaptured(vk);
        return true;
    }

    auto it = m_vkToFields.find(vk);
    if (it != m_vkToFields.end() && !it->second.empty())
    {
        // Trigger all handlers for this key
        for (auto* field : it->second)
        {
            field->trigger();
        }
        return true;
    }

    return false;
}
