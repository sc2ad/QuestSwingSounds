#pragma once
#include <dlfcn.h>
#include "../extern/beatsaber-hook/shared/utils/logging.hpp"
#include "config.hpp"
#include "../extern/beatsaber-hook/shared/utils/utils.h"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/typedefs.h"

Configuration& getConfig();
const Logger& logger();

class SliceSoundConfig;

class AudioManager {
    public:
        SliceSoundConfig config;

        void Initialize();
        void Reload();
        bool LoadAllAudio();
        Il2CppObject* GetNextAudioClip();
    private:
        int currentClipIndex = 0;
        int weightSum = -1;
        bool checkedWeights = false;
        static std::vector<Il2CppObject*> loadedAudioClips;

        bool loadAudioClip(std::string path);
        int getAudioType(std::string path);
        static void audioClipLoaded(Il2CppObject* webRequest);
};