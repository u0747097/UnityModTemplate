#pragma once

#include "core/serializable.h"
#include "config_manager.h"
#include "core/events/event_manager.h"
#include "fields/field_registry.h"

namespace config
{
    template <typename Derived>
    class ConfigObject : public ISerializable
    {
    public:
        ConfigObject(const std::string& section, const std::string& name)
            : m_section(section)
            , m_name(name)
            , m_path(section + "." + name)
        {
        }

        std::string getPath() const override
        {
            return m_path;
        }

        void serialize(nlohmann::json& json) const override
        {
            FieldRegistry::getInstance().serializeFields(m_path, json);
        }

        void deserialize(const nlohmann::json& json) override
        {
            FieldRegistry::getInstance().deserializeFields(m_path, json);
        }

        void save()
        {
            nlohmann::json json;
            serialize(json);

            auto& config = ConfigManager::getInstance();

            for (auto& [key, value] : json.items())
            {
                config.setFeatureValue(m_section, m_name, key, value);
            }
            config.scheduleSave();
        }

        void load()
        {
            auto& config = ConfigManager::getInstance();

            auto features = config.data()["features"];
            if (features.contains(m_section) && features[m_section].contains(m_name))
            {
                // Deserialize from the specific feature node
                deserialize(features[m_section][m_name]);
            }
            else
            {
                // Use default if no data is found
                nlohmann::json empty = nlohmann::json::object();
                deserialize(empty);
            }
        }

    protected:
        std::string m_section;
        std::string m_name;
        std::string m_path;
    };;
}
