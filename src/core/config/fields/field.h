#pragma once

#include "field_base.h"
#include "field_registry.h"
#include "core/config/config_manager.h"

namespace config
{
    template <typename T>
    class Field : public FieldBase
    {
    public:
        template <typename TRef>
        class Ref
        {
        public:
            Ref(TRef& value, std::function<void(const TRef&)> onChange)
                : m_value(value)
                , m_originalValue(value)
                , m_onChange(std::move(onChange))
            {
            }

            ~Ref()
            {
                if (m_value != m_originalValue && m_onChange)
                {
                    m_onChange(m_originalValue);
                }
            }

            operator TRef&() { return m_value; }
            TRef* operator->() { return &m_value; }
            TRef& operator*() { return m_value; }

        private:
            std::reference_wrapper<TRef> m_value;
            TRef m_originalValue;
            std::function<void(const TRef&)> m_onChange;
        };

        using ValueType = T;
        using Validator = std::function<bool(const T& value)>;
        using Connection = typename Event<const T&, const T&>::Connection;

        Field(const std::string& ownerPath, const std::string& key, T defaultValue)
            : m_ownerPath(ownerPath)
            , m_key(key)
            , m_value(std::move(defaultValue))
            , m_defaultValue(m_value)
            , m_dirty(false)
        {
            FieldRegistry::getInstance().registerField(m_ownerPath, this);
            loadFromConfig();
        }

        ~Field() override
        {
            FieldRegistry::getInstance().unregisterField(m_ownerPath, this);
        }

        const T& get() const { return m_value; }
        operator const T&() const { return m_value; }
        const T& operator*() const { return m_value; }
        const T* operator->() const { return &m_value; }

        // Get reference with automatic change detection
        auto ref()
        {
            return Ref<T>(m_value, [this](const T& originalValue)
            {
                validateAndNotify(originalValue);
            });
        }

        // Get direct reference without change detection
        T& direct() { return m_value; }

        Field& operator=(const T& newValue)
        {
            set(newValue);
            return *this;
        }

        void set(const T& newValue, bool suppressEvents = false)
        {
            if (m_validator && !m_validator(newValue))
            {
                LOG_WARN("Validation failed for field '{}', value rejected", m_key);
                return;
            }

            T oldValue = m_value;
            if (oldValue != newValue)
            {
                m_value = newValue;
                m_dirty = true;

                if (!suppressEvents)
                {
                    m_onChanged(oldValue, newValue);
                }

                saveToConfig();
            }
        }

        Connection onChanged(std::function<void(const T&, const T&)> handler)
        {
            return m_onChanged.connect(std::move(handler));
        }

        template <auto MemPtr, typename Instance>
        Connection onChanged(Instance* instance)
        {
            return m_onChanged.template connect<MemPtr>(instance);
        }

        template <auto FuncPtr>
        Connection onChanged()
        {
            return m_onChanged.template connect<FuncPtr>();
        }

        void setValidator(Validator validator)
        {
            m_validator = std::move(validator);
        }

        _NODISCARD bool isValid() const
        {
            return !m_validator || m_validator(m_value);
        }

        void resetToDefault() override
        {
            set(m_defaultValue);
        }

        _NODISCARD bool isDefault() const
        {
            return m_value == m_defaultValue;
        }

        const T& getDefault() const
        {
            return m_defaultValue;
        }

        void serialize(nlohmann::json& json) const override
        {
            json[m_key] = m_value;
        }

        void deserialize(const nlohmann::json& json) override
        {
            // LOG_DEBUG("deserialize: {}", json.dump(4));
            if (const auto it = json.find(m_key); it != json.end())
            {
                try
                {
                    T newValue = it->get<T>();
                    // Suppress events during deserialization to avoid unnecessary notifications
                    set(newValue, true);
                    m_dirty = false;
                }
                catch (const std::exception& e)
                {
                    LOG_WARN("Failed to deserialize field '{}': {}", m_key, e.what());
                    m_value = m_defaultValue;
                    m_dirty = false;
                }
            }
            else
            {
                // LOG_WARN("Field '{}' not found in JSON, using default value", m_key);
                m_value = m_defaultValue;
                m_dirty = false;
            }
        }

        std::string getKey() const override
        {
            return m_key;
        }

        _NODISCARD bool hasChanged() const override
        {
            return m_dirty;
        }

        void markClean() override
        {
            m_dirty = false;
        }

        void reload() override
        {
            loadFromConfig();
        }

    private:
        std::string m_ownerPath;
        std::string m_key;
        T m_value;
        T m_defaultValue;
        bool m_dirty;

        Validator m_validator;

        Event<const T&, const T&> m_onChanged;

        void validateAndNotify(const T& originalValue)
        {
            if (m_validator && !m_validator(m_value))
            {
                LOG_WARN("Validation failed for field '{}', reverting", m_key);
                m_value = originalValue;
                return;
            }

            if (originalValue != m_value)
            {
                m_dirty = true;
                m_onChanged(originalValue, m_value);
                saveToConfig();
            }
        }

        void loadFromConfig()
        {
            auto& config = ConfigManager::getInstance();
            T loaded = config.getFeatureValue<T>(extractSection(), extractFeature(), m_key, m_defaultValue);

            // Validate loaded value
            if (m_validator && !m_validator(loaded))
            {
                LOG_WARN("Loaded value for field '{}' failed validation, using default", m_key);
                loaded = m_defaultValue;
            }

            // Set without triggering events
            if (m_value != loaded)
            {
                m_value = loaded;
                m_dirty = false;
            }
        }

        void saveToConfig()
        {
            auto& config = ConfigManager::getInstance();
            config.setFeatureValue(extractSection(), extractFeature(), m_key, m_value);
            config.scheduleSave();
        }

        std::string extractSection() const
        {
            auto pos = m_ownerPath.find('.');
            return pos != std::string::npos ? m_ownerPath.substr(0, pos) : "Default";
        }

        std::string extractFeature() const
        {
            auto pos = m_ownerPath.find('.');
            return pos != std::string::npos ? m_ownerPath.substr(pos + 1) : m_ownerPath;
        }
    };
}
