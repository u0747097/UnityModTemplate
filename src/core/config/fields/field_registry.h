#pragma once

#include "field_base.h"

namespace config
{
    class FieldRegistry
    {
    public:
        static FieldRegistry& getInstance()
        {
            static FieldRegistry instance;
            return instance;
        }

        void registerField(const std::string& path, FieldBase* field)
        {
            std::lock_guard lock(m_mutex);
            m_fields[path].push_back(field);
        }

        void unregisterField(const std::string& path, FieldBase* field)
        {
            std::lock_guard lock(m_mutex);

            auto it = m_fields.find(path);
            if (it != m_fields.end())
            {
                auto& vec = it->second;
                std::erase(vec, field);
                if (vec.empty())
                {
                    m_fields.erase(it);
                }
            }
        }

        std::vector<FieldBase*> getFields(const std::string& path) const
        {
            std::lock_guard lock(m_mutex);

            auto it = m_fields.find(path);
            return it != m_fields.end() ? it->second : std::vector<FieldBase*>{};
        }

        std::vector<FieldBase*> getAllFields() const
        {
            std::lock_guard lock(m_mutex);

            std::vector<FieldBase*> allFields;
            for (const auto& fields : m_fields | std::views::values)
            {
                allFields.insert(allFields.end(), fields.begin(), fields.end());
            }
            return allFields;
        }

        void serializeFields(const std::string& path, nlohmann::json& json) const
        {
            for (auto* field : getFields(path))
            {
                field->serialize(json);
            }
        }

        void deserializeFields(const std::string& path, const nlohmann::json& json)
        {
            for (auto* field : getFields(path))
            {
                field->deserialize(json);
            }
        }

        void reloadAllFields() const
        {
            auto allFields = getAllFields();
            for (auto* field : allFields)
            {
                field->reload();
            }
        }

    private:
        mutable std::mutex m_mutex;
        std::unordered_map<std::string, std::vector<FieldBase*>> m_fields;
    };
}
