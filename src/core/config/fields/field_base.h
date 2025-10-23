#pragma once

namespace config
{
    class FieldBase
    {
    public:
        virtual ~FieldBase() = default;

        virtual void serialize(nlohmann::json& json) const = 0;
        virtual void deserialize(const nlohmann::json& json) = 0;
        virtual std::string getKey() const = 0;
        virtual bool hasChanged() const = 0;
        virtual void markClean() = 0;
        virtual void resetToDefault() = 0;
        virtual void reload() = 0;
    };
}
