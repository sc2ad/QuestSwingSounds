#pragma once
#define RAPIDJSON_HAS_STDSTRING 1
#include "../extern/beatsaber-hook/shared/config/config-utils.hpp"

class SliceSoundConfig {
    public:
        int majorVersion;
        int minorVersion;
        int patchVersion;
        bool enabled;
        std::vector<std::string> audioPaths;
        std::vector<int> weights;
        float speedThreshold;
        float volume;
        bool scaleVolumeWithSpeed; // Overrides volume
        bool randomize;
        bool playFirst; // Overrides randomize
        bool playBuffered;

        bool VersionGreaterThanEqual(int major, int minor, int patch);
        bool VersionLessThanEqual(int major, int minor, int patch);
        void WriteToConfig(ConfigDocument& config);
        void SetToDefault();
};

class ConfigHelper {
    public:
        static void RestoreConfig(std::string_view newPath);
        static void BackupConfig(ConfigDocument& config, std::string_view newPath);
        static SliceSoundConfig LoadConfig(ConfigDocument& config);
};
