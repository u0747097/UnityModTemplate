#pragma once

class ISerializable
{
public:
    virtual ~ISerializable() = default;

    virtual void serialize(nlohmann::json& json) const = 0;
    virtual void deserialize(const nlohmann::json& json) = 0;
    virtual std::string getPath() const = 0;
};
