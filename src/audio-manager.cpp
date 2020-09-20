#include "../include/config.hpp"
#include "../include/main.hpp"
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

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
    auto* audioClip = CRASH_UNLESS(il2cpp_utils::RunMethod("UnityEngine.Networking", "DownloadHandlerAudioClip", "GetContent", webRequest));
    CRASH_UNLESS(audioClip);
    loadedAudioClips.push_back(audioClip);
}

bool AudioManager::loadAudioClip(std::string path) {
    logger().info("Loading clip: %s", path.data());
    if (!fileexists(path.data())) {
        logger().error("File: %s does not exists!", path.data());
        return false;
    }
    auto audioType = getAudioType(path);
    logger().info("Audio type: %d", audioType);
    auto requestPath = il2cpp_utils::createcsstr("file:///" + path);
    auto* webRequest = CRASH_UNLESS(il2cpp_utils::RunMethod("UnityEngine.Networking", "UnityWebRequestMultimedia", "GetAudioClip", requestPath, audioType));
    static auto actionType = il2cpp_functions::class_get_type(il2cpp_utils::GetClassFromName("System", "Action"));
    auto action = CRASH_UNLESS(il2cpp_utils::MakeAction(actionType, webRequest, audioClipLoaded));
    Il2CppObject* asyncOperation = CRASH_UNLESS(il2cpp_utils::RunMethod(webRequest, "SendWebRequest"));
    if (!il2cpp_utils::SetFieldValue(asyncOperation, "m_completeCallback", action))
    {
        logger().error("Couldn't set completeCallback action!");
        return false;
    }
    logger().info("Began loading audio clip!");
    return true;
}

Il2CppObject* AudioManager::GetNextAudioClip() {
    logger().info("Getting next AudioClip! enabled: %s", config.enabled ? "true" : "false");
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
    logger().info("Loading all audio!");
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
    logger().debug("Initializing AudioManager!");
    currentClipIndex = 0;
    checkedWeights = false;
    logger().debug("Loading config!");
    getConfig().Load();
    config = ConfigHelper::LoadConfig(getConfig().config);
    logger().debug("Loaded config!");
    logger().debug("Completed Initialization of AudioManager!");
}

void AudioManager::Reload() {
    logger().debug("Reloading config!");
    getConfig().Reload();
    config = ConfigHelper::LoadConfig(getConfig().config);
    config.WriteToConfig(getConfig().config);
    logger().debug("Reloaded config!");
}