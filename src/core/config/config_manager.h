#pragma once

class ConfigManager
{
public:
    static ConfigManager& getInstance();

    bool load();
    bool save();
    void scheduleSave(int debounceMs = 250, bool reload = false);

    template <typename T>
    T getFeatureValue(const std::string& section, const std::string& name,
                      const std::string& key, const T& defaultValue) const
    {
        std::lock_guard lock(m_dataMutex);

        try
        {
            const auto* node = getFeatureNodeConst(section, name);
            if (!node) return defaultValue;

            auto it = node->find(key);
            if (it == node->end()) return defaultValue;

            return it->get<T>();
        }
        catch (const std::exception& e)
        {
            LOG_WARN("Failed to get feature value {}.{}.{}: {}", section, name, key, e.what());
            return defaultValue;
        }
    }

    template <typename T>
    void setFeatureValue(const std::string& section, const std::string& name,
                         const std::string& key, const T& value)
    {
        std::lock_guard lock(m_dataMutex);

        try
        {
            auto& node = getOrCreateFeatureNode(section, name);
            node[key] = value;
            markDirty();
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to set feature value {}.{}.{}: {}", section, name, key, e.what());
        }
    }

    void setProfile(const std::string& profileName);

    std::string getProfile() const
    {
        std::lock_guard lock(m_dataMutex);
        return m_currentProfile;
    }

    std::vector<std::string> listProfiles() const;
    bool createProfile(const std::string& profileName);
    bool deleteProfile(const std::string& profileName);

    void resetFeature(const std::string& section, const std::string& name);
    void resetAll();

    std::string getConfigFilePath() const { return getConfigPath(); }
    bool isDirty() const { return m_isDirty.load(); }
    const nlohmann::json& data() const { return m_data; }

private:
    ConfigManager() = default;

    mutable std::recursive_mutex m_dataMutex;
    nlohmann::json m_data;
    std::string m_currentProfile{"default"};
    std::atomic<bool> m_isDirty{false};

    std::atomic<uint64_t> m_saveSequence{0};
    mutable std::mutex m_saveMutex;

    std::string getConfigPath() const;
    std::filesystem::path getConfigDirectory() const;
    std::string profileNameFromFile(const std::string& filename) const;

    void markDirty() { m_isDirty.store(true); }
    void markClean() { m_isDirty.store(false); }
    bool createBackup() const;

    const nlohmann::json* getFeatureNodeConst(const std::string& section, const std::string& name) const;
    nlohmann::json* getFeatureNode(const std::string& section, const std::string& name);
    nlohmann::json& getOrCreateFeatureNode(const std::string& section, const std::string& name);
    void initializeEmptyConfig();
    bool validateConfig() const;
};
