#pragma once

#include "hook_manager.h"

template <typename Signature>
class FunctionHook;

template <typename ReturnType, typename... Args>
class FunctionHook<ReturnType(Args...)>
{
public:
    using FunctionPtr = ReturnType(*)(Args...);

    explicit FunctionHook(FunctionPtr target = nullptr)
        : m_targetFn(target)
        , m_targetSet(target != nullptr)
    {
    }

    void target(FunctionPtr target)
    {
        if (!m_isHooked && !m_targetSet)
        {
            m_targetFn = target;
            m_targetSet = true;
        }
    }

    bool set(FunctionPtr detour)
    {
        if (m_isHooked || !m_targetSet || !m_targetFn) return false;

        m_detourFn = reinterpret_cast<void*>(detour);

        bool success = HookManager::install(m_targetFn, detour);

        if (success)
        {
            m_isHooked = true;
            m_originalFn = HookManager::getInstance().getOriginal(detour);
        }

        return success;
    }

    // TODO: Support lambda hooks later
    // bool set() 
    // {
    // }

    ReturnType call(Args... args) const
    {
        if (m_isHooked && m_originalFn)
        {
            auto original = reinterpret_cast<FunctionPtr>(m_originalFn);
            return original(args...);
        }

        if (m_targetFn)
        {
            return m_targetFn(args...);
        }

        if constexpr (std::is_void_v<ReturnType>)
        {
            return;
        }
        else
        {
            return ReturnType{};
        }
    }

    // FunctionPtr original() const
    // {
    //     if (m_isHooked && m_originalFn)
    //     {
    //         return reinterpret_cast<FunctionPtr>(m_originalFn);
    //     }
    //     return m_targetFn;
    // }

    void remove()
    {
        if (m_isHooked && m_targetFn)
        {
            HookManager::getInstance().uninstall(reinterpret_cast<void*>(m_targetFn));
            m_isHooked = false;
            m_originalFn = nullptr;
            m_detourFn = nullptr;
        }
    }

    void enable(const bool enabled = true)
    {
        if (!m_targetFn) return;

        if (enabled && !m_isHooked)
        {
            HookManager::getInstance().enableHook(reinterpret_cast<void*>(m_targetFn));
        }
        else if (!enabled && m_isHooked)
        {
            HookManager::getInstance().disableHook(reinterpret_cast<void*>(m_targetFn));
        }
    }

    void disable() { enable(false); }

    _NODISCARD bool isActive() const { return m_isHooked; }
    _NODISCARD bool hasTarget() const { return m_targetSet && m_targetFn != nullptr; }

private:
    FunctionPtr m_targetFn = nullptr;
    void* m_detourFn = nullptr;
    void* m_originalFn = nullptr;
    bool m_isHooked = false;
    bool m_targetSet = false;
};
