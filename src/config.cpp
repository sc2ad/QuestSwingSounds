#include "../include/config.hpp"
#include "../extern/beatsaber-hook/shared/utils/logging.h"
#include "../extern/beatsaber-hook/rapidjson/include/rapidjson/allocators.h"
#include "../extern/beatsaber-hook/rapidjson/include/rapidjson/document.h"

std::vector<std::string> getAudioPaths(rapidjson::Value& arr) {
    std::vector<std::string> out;
    auto size = arr.Size();
    for (int i = 0; i < size; i++) {
        out.push_back(std::string(arr[i].GetString()));
    }
    return out;
}

std::vector<int> getWeights(rapidjson::Value& arr) {
    std::vector<int> out;
    auto size = arr.Size();
    for (int i = 0; i < size; i++) {
        out.push_back(arr[i].GetInt());
    }
    return out;
}

bool SliceSoundConfig::VersionLessThanEqual(int major, int minor, int patch) {
    return major > majorVersion || (major == majorVersion && (minor > minorVersion || (minor == minorVersion && patch >= patchVersion)));
}

bool SliceSoundConfig::VersionGreaterThanEqual(int major, int minor, int patch) {
    return major < majorVersion || (major == majorVersion && (minor < minorVersion || (minor == minorVersion && patch <= patchVersion)));
}

SliceSoundConfig ConfigHelper::LoadConfig(ConfigDocument& config) {
    SliceSoundConfig con;

    if (!config.IsObject()) {
        con.SetToDefault();
        return con;
    }
    con.majorVersion = config["majorVersion"].GetInt();
    con.minorVersion = config["minorVersion"].GetInt();
    con.patchVersion = config["patchVersion"].GetInt();
    con.enabled = config["enabled"].GetBool();
    con.audioPaths = getAudioPaths(config["audioPaths"]);
    auto itr = config.FindMember("weights");
    auto end = config.MemberEnd();
    if (itr != end) {
        con.weights = getWeights(itr->value);
    } else {
        con.weights = std::vector<int>();
    }
    con.speedThreshold = config["speedThreshold"].GetFloat();
    con.volume = config["volume"].GetFloat();
    con.scaleVolumeWithSpeed = config["scaleVolumeWithSpeed"].GetBool();
    con.playBuffered = config["playBuffered"].GetBool();
    con.playFirst = config["playFirst"].GetBool();
    con.randomize = config["randomize"].GetBool();
    return con;
}

void SliceSoundConfig::WriteToConfig(ConfigDocument& config) {
    log(DEBUG, "Starting to write to config");
    config.SetObject();
    config.RemoveAllMembers();
    rapidjson::MemoryPoolAllocator<>& allocator = config.GetAllocator();
    // Add versions
    config.AddMember("majorVersion", majorVersion, allocator);
    config.AddMember("minorVersion", minorVersion, allocator);
    config.AddMember("patchVersion", patchVersion, allocator);
    config.AddMember("enabled", enabled, allocator);
    auto arr = rapidjson::Value(rapidjson::kArrayType);
    for (auto itr = audioPaths.begin(); itr != audioPaths.end(); ++itr) {
        arr.PushBack(rapidjson::GenericStringRef<char>((*itr).data()), allocator);
    }
    config.AddMember("audioPaths", arr, allocator);
    arr = rapidjson::Value(rapidjson::kArrayType);
    for (auto itr = weights.begin(); itr != weights.end(); ++itr) {
        arr.PushBack(*itr, allocator);
    }
    config.AddMember("weights", arr, allocator);
    config.AddMember("speedThreshold", speedThreshold, allocator);
    config.AddMember("volume", volume, allocator);
    config.AddMember("scaleVolumeWithSpeed", scaleVolumeWithSpeed, allocator);
    config.AddMember("playBuffered", playBuffered, allocator);
    config.AddMember("playFirst", playFirst, allocator);
    config.AddMember("randomize", randomize, allocator);
    log(DEBUG, "Wrote config to document!");
}

void SliceSoundConfig::SetToDefault() {
    log(DEBUG, "Setting config to default!");
    majorVersion = 0;
    minorVersion = 1;
    patchVersion = 0;
    enabled = true;
    audioPaths = std::vector<std::string>({std::string("/sdcard/Android/data/com.beatgames.beatsaber/files/slice_sounds/oneRandomSound.ogg")});
    weights = std::vector<int>();
    speedThreshold = 0;
    volume = 0.8;
    scaleVolumeWithSpeed = true;
    playBuffered = true;
    playFirst = false;
    randomize = false;
    log(DEBUG, "Set to default!");
}