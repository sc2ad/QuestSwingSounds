#include "../include/config.hpp"
#include "../include/main.hpp"
#include "../extern/beatsaber-hook/shared/utils/logging.h"

std::vector<Il2CppObject*> AudioManager::loadedAudioClips;

// Gets the audio type from a path
int AudioManager::getAudioType(std::string path) {
    if (path.ends_with(".ogg")) {
        return 0xE;
    } else if (path.ends_with(".wav")) {
        return 0x14;
    } else if (path.ends_with(".mp3")) {
        return 0xD;
    }
    return 0;
}

void AudioManager::audioClipLoaded(Il2CppObject* webRequest) {
    Il2CppObject* audioClip;
    if (il2cpp_utils::RunMethod(&audioClip, il2cpp_utils::GetClassFromName("UnityEngine.Networking", "DownloadHandlerAudioClip"), "GetContent", webRequest)) {
        loadedAudioClips.push_back(audioClip);
    } else {
        log(ERROR, "Could not GetContent from AudioClip!");
    }
}

bool AudioManager::loadAudioClip(std::string path) {
    log(INFO, "Loading clip: %s", path.data());
    if (!fileexists(path.data())) {
        log(ERROR, "File: %s does not exists!", path.data());
        return false;
    }
    auto audioType = getAudioType(path);
    log(INFO, "Audio type: %d", audioType);
    auto requestPath = il2cpp_utils::createcsstr("file:///" + path);
    Il2CppObject* webRequest;
    if (!il2cpp_utils::RunMethod(&webRequest, il2cpp_utils::GetClassFromName("UnityEngine.Networking", "UnityWebRequestMultimedia"), "GetAudioClip", requestPath, audioType)) {
        log(ERROR, "Could not run GetAudioClip on request: %s", to_utf8(csstrtostr(requestPath)).data());
        return false;
    }
    static auto actionType = il2cpp_functions::class_get_type(il2cpp_utils::GetClassFromName("System", "Action"));
    auto action = il2cpp_utils::MakeAction(webRequest, audioClipLoaded, actionType);
    if (action == nullptr) {
        log(ERROR, "Could not create action!");
        return false;
    }
    Il2CppObject* asyncOperation;
    if (!il2cpp_utils::RunMethod(&asyncOperation, webRequest, "SendWebRequest")) {
        log(ERROR, "Could not send web request!");
        return false;
    }
    if (!il2cpp_utils::SetFieldValue(asyncOperation, "m_completeCallback", action))
    {
        log(ERROR, "Couldn't set completeCallback action!");
        return false;
    }
    log(INFO, "Began loading audio clip!");
    return true;
}

Il2CppObject* AudioManager::GetNextAudioClip() {
    log(INFO, "Getting next AudioClip! enabled: %s", config.enabled ? "true" : "false");
    if (!config.enabled) {
        return nullptr;
    }
    if (loadedAudioClips.size() == 0) {
        return nullptr;
    }
    if (config.playFirst) {
        return loadedAudioClips[0];
    }
    if (config.randomize) {
        // Performs a uniform distribution, UNLESS the weights vector is of the same length as the LOADED size
        // This means that all audio clips MUST load for weights to be used
        // weightSum is setup beforehand to be the sum of the weights vector
        if (!checkedWeights) {
            checkedWeights = true;
            if (loadedAudioClips.size() == config.weights.size()) {
                weightSum = 0;
                for (auto itr = config.weights.begin(); itr != config.weights.end(); ++itr) {
                    weightSum += *itr;
                }
            }
        }
        if (weightSum != -1) {
            
            // TODO: Make this a binary search for large weight arrays
            int value = rand() % weightSum;
            for (int i = 0; i < loadedAudioClips.size(); ++i) {
                if (value < config.weights[i]) {
                    return loadedAudioClips[i];
                }
                value -= config.weights[i];
            }
            return nullptr;
        }
        return loadedAudioClips[rand() % loadedAudioClips.size()];
    }
    // Iterate in sequence
    auto tmp = loadedAudioClips[currentClipIndex];
    currentClipIndex = (currentClipIndex + 1) % loadedAudioClips.size();
    return tmp;
}

// Creates webrequests for obtaining each AudioClip
bool AudioManager::LoadAllAudio() {
    log(INFO, "Loading all audio!");
    if (!config.enabled) {
        return true;
    }
    for (auto itr = config.audioPaths.begin(); itr != config.audioPaths.end(); ++itr) {
        if (!loadAudioClip(*itr)) {
            return false;
        }
    }
    return true;
}

void AudioManager::Initialize() {
    log(DEBUG, "Initializing AudioManager!");
    currentClipIndex = 0;
    checkedWeights = false;
    log(DEBUG, "Loading config!");
    Configuration::Load();
    config = ConfigHelper::LoadConfig(Configuration::config);
    log(DEBUG, "Loaded config!");
    log(DEBUG, "Completed Initialization of AudioManager!");
}

void AudioManager::Reload() {
    log(DEBUG, "Reloading config!");
    Configuration::Load();
    config = ConfigHelper::LoadConfig(Configuration::config);
    config.WriteToConfig(Configuration::config);
    log(DEBUG, "Reloaded config!");
}