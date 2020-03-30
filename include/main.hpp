#pragma once
#include <dlfcn.h>
#include "../extern/beatsaber-hook/shared/utils/logging.h"
#include "config.hpp"
#include "../extern/beatsaber-hook/shared/utils/utils.h"

extern "C" void load();

class SliceSoundConfig;

class AudioManager {
    public:
        SliceSoundConfig config;

        void Initialize();
        void Reload();
        bool LoadAllAudio();
        Il2CppObject* GetNextAudioClip();
    private:
        int currentClipIndex;
        int weightSum = -1;
        bool checkedWeights;
        static std::vector<Il2CppObject*> loadedAudioClips;

        bool loadAudioClip(std::string path);
        int getAudioType(std::string path);
        static void audioClipLoaded(Il2CppObject* webRequest);
};