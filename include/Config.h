#pragma once 

#include "REX/REX.h"

#include <unordered_map>
#include <unordered_set>
#include <nlohmann/json.hpp>
#include <shared_mutex>

class Config : public REX::Singleton<Config> {
public:
    Config() { LoadConfig(); }

    void LoadConfig() {
        LoadIni();
        LoadJsons();

        std::shared_lock lock(mutex_);
        logger::info("Loaded {} wind configs", windConfig_.size());
        logger::info("Default strength: {}", defaultStrength_);
    }

    float GetStrength(const RE::NiParticleSystem* ps) {
        if (!ps) {
            return 0.0f;
        }
        std::vector<std::string> hierarchy;

        const RE::NiAVObject* current = ps;

        std::shared_lock lock(mutex_);
        while (current) {
            std::string name = "[unnamed]";
            if (current->name.c_str()) {
                name = current->name.c_str();
            }


            auto it = windConfig_.find(name);
            if (it != windConfig_.end()) {
                std::unique_lock logLock(logMutex_);
                if (_loggedConfigHits.insert(name).second) {
                    logger::info("Using CONFIG strength {} for '{}'", it->second, name);
                }
                return it->second;
            }
            hierarchy.emplace_back(name);
            current = current->parent;
        }

        std::string path;
        for (auto& name : hierarchy)
        {
            if (path.empty()) {
                path = name;
            } else {
                path = name + ">" + path;
            }

            auto itH = windConfig_.find(path);
            if (itH != windConfig_.end()) {
                std::unique_lock logLock(logMutex_);
                if (_loggedConfigHits.insert(path).second) {
                    logger::info("Using CONFIG strength: {} for hierarchy:", itH->second);
                    logger::info("{}", path);
                }
                return itH->second;
            }
        }

        for (const auto& [pattern, strength] : wildcardConfig_) {
            if (path.contains(pattern)) {
                std::unique_lock logLock(logMutex_);
                if (_loggedConfigHits.insert(pattern).second) {
                    logger::info("Using WILDCARD strength: {} for hierarchy:", strength);
                    logger::info("{}", path);
                }
                return strength;
            }
        }

        std::unique_lock logLock(logMutex_);
        if (_loggedHierarchies.insert(path).second) {
            logger::info("Using default strength: {} for particle hierarchy:", defaultStrength_);
            logger::info("{}", path);
        }
        return defaultStrength_;
    }

    void LoadIni() {
        std::ifstream file("Data\\SKSE\\Plugins\\ParticleWind.ini");
        if (!file.is_open()) {
            logger::error("INI not found, using default strength {}", defaultStrength_);
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.contains("fDefaultStrength")) {
                auto pos = line.find('=');
                if (pos != std::string::npos) {
                    defaultStrength_ = std::stof(line.substr(pos + 1));
                }
            }
            if (line.contains("bWindDirectionFix")) {
                auto pos = line.find('=');
                if (pos != std::string::npos) {
                    std::string value = line.substr(pos + 1);
                    windDirectionFix_ = (value == "true" || value == "True" || value == "1");
                }
            }
        }
    }

    void LoadJsons() {
        const std::string& folder = "Data\\SKSE\\Plugins\\ParticleWind\\";
        namespace fs = std::filesystem;

        if (!fs::exists(folder)) {
            logger::error("Config folder not found: {}", folder);
            return;
        }

        for (const auto& entry : fs::directory_iterator(folder)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;

            std::ifstream file(entry.path());
            if (!file.is_open()) {
                logger::warn("Failed to open {}", entry.path().string());
                continue;
            }

            try {
                nlohmann::json j;
                file >> j;

                std::unique_lock lock(mutex_);
                for (auto& [key, value] : j.items()) {
                    float strength = value.get<float>();
                    std::string k = key;
                    if (k.size() >= 2 && k.front() == '*' && k.back() == '*') {
                        k = k.substr(1, k.size() - 2);  // strip '*' from both ends
                        wildcardConfig_[k] = strength;
                    } else {
                        windConfig_[k] = strength;
                    }
                }

                logger::info("Loaded {}", entry.path().string());
            } catch (const std::exception& e) {
                logger::error("JSON error in {}: {}", entry.path().string(), e.what());
            }
        }
    }

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, float> windConfig_;
    std::unordered_map<std::string, float> wildcardConfig_;

    
    mutable std::shared_mutex logMutex_;
    std::unordered_set<std::string> _loggedHierarchies;
    std::unordered_set<std::string> _loggedConfigHits;

    bool windDirectionFix_{true};
    float defaultStrength_{5.0f};
};