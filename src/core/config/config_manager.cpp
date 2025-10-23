#include "pch.h"
#include "config_manager.h"

#ifndef CONFIG_VERSION
#define CONFIG_VERSION 2 // To other devs: Update this if you change the config structure plz

#include "core/events/event_manager.h"
#include "fields/field_registry.h"

ConfigManager& ConfigManager::getInstance()
{
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::load()
{
    std::lock_guard lock(m_dataMutex);

    const auto path = getConfigPath();
    // LOG_DEBUG("Loading config from: {}", path);

    // Create backup before loading
    if (std::filesystem::exists(path))
    {
        createBackup();
    }

    std::ifstream file(path);
    if (!file.good())
    {
        LOG_INFO("Config file not found for profile '{}', creating default configuration", m_currentProfile);
        initializeEmptyConfig();
        markDirty();
        return true;
    }

    try
    {
        file >> m_data;

        // Validate and fix structure if needed
        if (!validateConfig())
        {
            LOG_WARN("Invalid config structure for profile '{}', fixing...", m_currentProfile);

            // Try to preserve existing data while fixing structure
            if (!m_data.contains("features"))
            {
                m_data["features"] = nlohmann::json::object();
            }
            if (!m_data["features"].is_object())
            {
                m_data["features"] = nlohmann::json::object();
            }
        }

        markClean();
        LOG_INFO("Configuration loaded successfully for profile '{}'", m_currentProfile);
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to parse config file: {}", e.what());

        // Try to restore from backup
        auto backupPath = path + ".backup";
        if (std::filesystem::exists(backupPath))
        {
            LOG_INFO("Attempting to restore from backup...");
            try
            {
                std::ifstream backupFile(backupPath);
                backupFile >> m_data;
                if (validateConfig())
                {
                    LOG_INFO("Successfully restored from backup");
                    markDirty();
                    return true;
                }
            }
            catch (...)
            {
                LOG_ERROR("Backup restoration failed");
            }
        }

        // Fallback to creating a fresh config
        LOG_WARN("Creating fresh configuration");
        initializeEmptyConfig();
        markDirty();
        return false;
    }
}

bool ConfigManager::save()
{
    std::lock_guard saveLock(m_saveMutex);
    std::lock_guard dataLock(m_dataMutex);

    const auto path = getConfigPath();

    try
    {
        auto dir = std::filesystem::path(path).parent_path();
        create_directories(dir);

        // Write to temporary file first
        auto tempPath = path + ".tmp";
        std::ofstream file(tempPath, std::ios::trunc);
        if (!file.good())
        {
            LOG_ERROR("Failed to open temp file for writing: {}", tempPath);
            return false;
        }

        file << m_data.dump(2);
        file.close();

        std::error_code ec;
        std::filesystem::rename(tempPath, path, ec);
        if (ec)
        {
            LOG_ERROR("Failed to rename temp file: {}", ec.message());
            // Try direct write as fallback
            std::ofstream directFile(path, std::ios::trunc);
            directFile << m_data.dump(2);
            directFile.close();
        }

        markClean();
        // LOG_DEBUG("Configuration saved to: {}", path);
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to save config: {}", e.what());
        return false;
    }
}

void ConfigManager::scheduleSave(int debounceMs, bool reload)
{
    const auto sequence = ++m_saveSequence;

    std::thread([this, sequence, debounceMs, reload]
    {
        // Wait for debounce period
        auto delay = max(0, debounceMs);
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));

        // Only save if this is still the latest request
        if (m_saveSequence.load() == sequence)
        {
            save();

            if (reload)
            {
                // Reload all registered fields after save
                config::FieldRegistry::getInstance().reloadAllFields();
                EventManager::onReloadConfig();
            }
        }
    }).detach();
}

void ConfigManager::setProfile(const std::string& profileName)
{
    if (profileName.empty())
    {
        LOG_WARN("Cannot set empty profile name");
        return;
    }

    std::lock_guard lock(m_dataMutex);

    if (profileName == m_currentProfile) return;

    if (isDirty())
    {
        LOG_INFO("Saving current profile '{}' before switching", m_currentProfile);
        save();
    }

    LOG_INFO("Switching from profile '{}' to '{}'", m_currentProfile, profileName);

    // Update profile name
    std::string previousProfile = m_currentProfile;
    m_currentProfile = profileName;

    if (!load())
    {
        LOG_ERROR("Failed to load profile '{}', reverting to '{}'", profileName, previousProfile);
        m_currentProfile = previousProfile;
        load();
        return;
    }

    // Reload all registered fields with new profile data
    config::FieldRegistry::getInstance().reloadAllFields();

    EventManager::onReloadConfig();

    LOG_INFO("Successfully switched to profile '{}'", m_currentProfile);
}

std::vector<std::string> ConfigManager::listProfiles() const
{
    std::vector<std::string> profiles;

    try
    {
        auto configDir = getConfigDirectory();

        for (const auto& entry : std::filesystem::directory_iterator(configDir))
        {
            if (!entry.is_regular_file()) continue;

            if (auto filename = entry.path().filename().string(); filename == "config.json")
            {
                profiles.emplace_back("default");
            }
            else if (filename.starts_with("config.") && filename.ends_with(".json"))
            {
                auto profileName = profileNameFromFile(filename);
                if (!profileName.empty() && profileName != "default")
                {
                    profiles.push_back(profileName);
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to list profiles: {}", e.what());
    }

    // Ensure default is always present
    if (std::ranges::find(profiles, "default") == profiles.end())
    {
        profiles.emplace_back("default");
    }

    std::ranges::sort(profiles);
    profiles.erase(std::ranges::unique(profiles).begin(), profiles.end());

    return profiles;
}

bool ConfigManager::createProfile(const std::string& profileName)
{
    if (profileName.empty() || profileName == "default")
    {
        LOG_ERROR("Invalid profile name: '{}'", profileName);
        return false;
    }

    // Check for invalid characters
    if (profileName.find_first_of("\\/:*?\"<>|") != std::string::npos)
    {
        LOG_ERROR("Profile name contains invalid characters: '{}'", profileName);
        return false;
    }

    std::lock_guard lock(m_dataMutex);

    auto previousProfile = m_currentProfile;
    m_currentProfile = profileName;

    try
    {
        initializeEmptyConfig();

        if (save())
        {
            LOG_INFO("Created new profile: '{}'", profileName);
            return true;
        }
        LOG_ERROR("Failed to save new profile: '{}'", profileName);
        m_currentProfile = previousProfile;
        return false;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception creating profile '{}': {}", profileName, e.what());
        m_currentProfile = previousProfile;
        return false;
    }
}

bool ConfigManager::deleteProfile(const std::string& profileName)
{
    if (profileName.empty() || profileName == "default")
    {
        LOG_ERROR("Cannot delete default profile");
        return false;
    }

    try
    {
        auto profilePath = getConfigDirectory() / ("config." + profileName + ".json");

        if (!exists(profilePath))
        {
            LOG_WARN("Profile '{}' does not exist", profileName);
            return false;
        }

        // Create backup before deletion
        auto backupPath = profilePath.string() + ".deleted";
        copy_file(profilePath, backupPath);

        std::filesystem::remove(profilePath);

        LOG_INFO("Deleted profile: '{}' (backup saved as {})", profileName, backupPath);

        if (m_currentProfile == profileName)
        {
            setProfile("default");
        }

        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to delete profile '{}': {}", profileName, e.what());
        return false;
    }
}

void ConfigManager::resetFeature(const std::string& section, const std::string& name)
{
    std::lock_guard lock(m_dataMutex);

    try
    {
        if (!m_data.contains("features")) return;

        auto& features = m_data["features"];
        if (!features.contains(section)) return;

        auto& sectionData = features[section];
        if (sectionData.contains(name))
        {
            sectionData.erase(name);
            markDirty();
            LOG_INFO("Reset feature: {}.{}", section, name);
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to reset feature {}.{}: {}", section, name, e.what());
    }
}

void ConfigManager::resetAll()
{
    std::lock_guard lock(m_dataMutex);

    LOG_WARN("Resetting entire configuration to defaults");
    initializeEmptyConfig();
    markDirty();
    scheduleSave(0, true);
}

std::string ConfigManager::getConfigPath() const
{
    auto configDir = getConfigDirectory();

    std::string filename = m_currentProfile == "default"
        ? "config.json"
        : "config." + m_currentProfile + ".json";


    return (configDir / filename).string();
}

std::filesystem::path ConfigManager::getConfigDirectory() const
{
    char* appData = nullptr;
    size_t len = 0;
    _dupenv_s(&appData, &len, "APPDATA");

    std::filesystem::path base = appData ? appData : ".";
    if (appData) free(appData);

    auto configDir = base / "Cunny";

    // Ensure directory exists
    std::error_code ec;
    if (!exists(configDir, ec))
    {
        create_directories(configDir, ec);
        if (ec)
        {
            LOG_ERROR("Failed to create config directory: {}", ec.message());
        }
    }

    return configDir;
}

std::string ConfigManager::profileNameFromFile(const std::string& filename) const
{
    if (filename == "config.json") return "default";

    const std::string prefix = "config.";
    const std::string suffix = ".json";

    if (filename.starts_with(prefix) && filename.ends_with(suffix))
    {
        auto start = prefix.length();
        auto length = filename.length() - prefix.length() - suffix.length();
        return filename.substr(start, length);
    }

    return "";
}

bool ConfigManager::createBackup() const
{
    try
    {
        auto configPath = getConfigPath();
        if (!std::filesystem::exists(configPath)) return true;

        auto backupPath = configPath + ".backup";
        copy_file(configPath, backupPath,
                  std::filesystem::copy_options::overwrite_existing);

        // LOG_DEBUG("Created config backup: {}", backupPath);
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_WARN("Failed to create config backup: {}", e.what());
        return false;
    }
}

const nlohmann::json* ConfigManager::getFeatureNodeConst(const std::string& section, const std::string& name) const
{
    auto featuresIt = m_data.find("features");
    if (featuresIt == m_data.end() || !featuresIt->is_object()) return nullptr;

    auto sectionIt = featuresIt->find(section);
    if (sectionIt == featuresIt->end() || !sectionIt->is_object()) return nullptr;

    auto featureIt = sectionIt->find(name);
    if (featureIt == sectionIt->end() || !featureIt->is_object()) return nullptr;

    return &*featureIt;
}

nlohmann::json* ConfigManager::getFeatureNode(const std::string& section, const std::string& name)
{
    auto featuresIt = m_data.find("features");
    if (featuresIt == m_data.end() || !featuresIt->is_object()) return nullptr;

    auto sectionIt = featuresIt->find(section);
    if (sectionIt == featuresIt->end() || !sectionIt->is_object()) return nullptr;

    auto featureIt = sectionIt->find(name);
    if (featureIt == sectionIt->end() || !featureIt->is_object()) return nullptr;

    return &*featureIt;
}

nlohmann::json& ConfigManager::getOrCreateFeatureNode(const std::string& section, const std::string& name)
{
    // Ensure features object exists
    if (!m_data.contains("features") || !m_data["features"].is_object())
    {
        m_data["features"] = nlohmann::json::object();
    }

    auto& features = m_data["features"];

    // Ensure section object exists
    if (!features.contains(section) || !features[section].is_object())
    {
        features[section] = nlohmann::json::object();
    }

    auto& sectionData = features[section];

    // Ensure feature object exists
    if (!sectionData.contains(name) || !sectionData[name].is_object())
    {
        sectionData[name] = nlohmann::json::object();
    }

    return sectionData[name];
}

void ConfigManager::initializeEmptyConfig()
{
    m_data = nlohmann::json{
        {"version", CONFIG_VERSION},
        {"features", nlohmann::json::object()},
        {"metadata", {
             {"created", std::time(nullptr)},
             {"profile", m_currentProfile}
         }}
    };
}

bool ConfigManager::validateConfig() const
{
    try
    {
        if (!m_data.is_object()) return false;
        if (!m_data.contains("features")) return false;
        if (!m_data["features"].is_object()) return false;

        for (const auto& [sectionName, section] : m_data["features"].items())
        {
            if (!section.is_object()) return false;

            for (const auto& [featureName, feature] : section.items())
            {
                if (!feature.is_object()) return false;
            }
        }

        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

#endif
