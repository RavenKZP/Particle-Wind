#pragma once 

#include "REX/REX.h"

#include <unordered_map>
#include <nlohmann/json.hpp>
#include <shared_mutex>

class Config : public REX::Singleton<Config> {
public:
    Config() { LoadConfig(); }

    void LoadConfig() {
        std::unique_lock lock(mutex_);
        LoadIni();
        LoadJsons();

        logger::info("Loaded {} wind configs", windConfig_.size());
        logger::info("Default strength: {}", defaultStrength_);
    }

    float GetStrength(const RE::NiParticleSystem* ps) {
        if (!ps) {
            return defaultStrength_;
        }
        std::vector<std::string> hierarchy;

        // Walk hierarchy (self -> root)
        // 1️. Try to find first match in hierarchy
        const RE::NiAVObject* current = ps;
        while (current) {
            if (auto name = current->name.c_str(); name && name[0] != '\0') {
                auto it = windConfig_.find(name);
                if (it != windConfig_.end()) {
                    return it->second;
                }
                hierarchy.emplace_back(name);
            } else {
                hierarchy.emplace_back("<unnamed>");
            }
            current = current->parent;
        }

        // 2️. Not found -> register self (original particle system name)
        const std::string& selfName = hierarchy.front();
        {
            std::unique_lock lock(mutex_);
            windConfig_[selfName] = defaultStrength_;
        }

        // 3️. Log
        logger::info("Using default strength: {} for particle hierarchy:", defaultStrength_);
        for (auto it = hierarchy.rbegin(); it != hierarchy.rend(); ++it) {
            size_t depth = std::distance(hierarchy.rbegin(), it);
            std::string indent(depth * 2, ' ');
            std::string prefix = (depth == 0) ? "" : "|-";
            logger::info("{}{}{}", indent, prefix, *it);
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

                for (auto& [key, value] : j.items()) {
                    float strength = value.get<float>();
                    windConfig_[std::string(key.c_str())] = strength;
                }

                logger::info("Loaded {}", entry.path().string());
            } catch (const std::exception& e) {
                logger::error("JSON error in {}: {}", entry.path().string(), e.what());
            }
        }
    }

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, float> windConfig_;
    float defaultStrength_{5.0f};
};