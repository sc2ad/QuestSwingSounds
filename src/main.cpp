#include "../include/config.hpp"
#include "../include/main.hpp"
#include "../extern/beatsaber-hook/shared/utils/logging.hpp"
#include "../extern/custom-ui/shared/customui.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

AudioManager manager;
CustomUI::TextObject alertText;

void InitializeAlertText() {
    alertText.fontSize = 5;
    alertText.anchoredPosition = (Vector2){0.0, -100.0};
}

void SetText(std::string_view text) {
    // TODO: Cache commonly used 'text' --> Il2CppString* conversions
    if (!alertText.set(text)) {
        logger().warning("Not displaying text: %s", text.data());
        return;
    }
}

void ReloadAlertText(Il2CppObject* mainMenu) {
    CRASH_UNLESS(mainMenu);
    logger().info("Reloading alert text!");
    alertText.parentTransform = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(mainMenu, "rectTransform"));
    if (alertText.gameObj == nullptr || alertText.textMesh == nullptr) {
        if (!alertText.create()) {
            logger().warning("Failed to create text object! Will not display text.");
        }
    }
}

static Il2CppObject* cachedLeftSaber;
static Il2CppObject* cachedRightSaber;

// Ensures the AudioSource exists on the saber, and returns it
Il2CppObject* EnsureAudioSource(Il2CppObject* saber, Il2CppObject** cachedSaber) {
    if (saber == nullptr) {
        logger().error("saber is null!");
        return nullptr;
    }
    static auto audioSourceTypeObject = il2cpp_utils::GetSystemType("UnityEngine", "AudioSource");
    if (saber != *cachedSaber) {
        // When the cachedSaber is not the same as the actual saber, ensure component exists
        auto* audioSourceComponent = CRASH_UNLESS(il2cpp_utils::RunMethod(saber, "GetComponent", audioSourceTypeObject));
        if (audioSourceComponent == nullptr) {
            audioSourceComponent = CRASH_UNLESS(il2cpp_utils::RunMethod(saber, "AddComponent", audioSourceTypeObject));
            CRASH_UNLESS(audioSourceComponent);
            il2cpp_utils::SetPropertyValue(audioSourceComponent, "loop", false);
            il2cpp_utils::SetPropertyValue(audioSourceComponent, "playOnAwake", false);
        }
        *cachedSaber = saber;
        return audioSourceComponent;
    } else {
        // Just get the component
        return CRASH_UNLESS(il2cpp_utils::RunMethod(saber, "GetComponent", audioSourceTypeObject));
    }
}

// Add an AudioSource component to the Saber (if they haven't already been added)
// Get Saber.bladeSpeed (property) and ensure it is over the threshold
// Play the sound if it is
void CheckSaber(Il2CppObject* saber, Il2CppObject* audioSourceComponent, Il2CppObject* audioClip) {
    if (saber == nullptr || audioSourceComponent == nullptr) {
        logger().error("saber or audioSourceComponent is null!");
        return;
    }
    // Check saber details
    float speed = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<float>(saber, "bladeSpeed"));
    logger().debug("Got saber.bladeSpeed: %f", speed);
    if (speed <= manager.config.speedThreshold) {
        logger().debug("Speed threshold not met");
        return;
    }
    
    // We know that audioSourceComponent must be non-null
    float volume = manager.config.scaleVolumeWithSpeed ? speed / 100.0f : manager.config.volume;
    if (!il2cpp_utils::RunMethod(audioSourceComponent, "PlayOneShot", audioClip, volume)) {
        logger().critical("Could not audioSourceComponent.PlayOneShot(audioClip, volume)!");
        return;
    }
}

MAKE_HOOK_OFFSETLESS(SaberManager_Update, void, Il2CppObject* self) {
    SaberManager_Update(self);    
    logger().debug("Getting next audio clip");
    auto clip = manager.GetNextAudioClip();
    logger().debug("Got audio clip!");
    logger().debug("Checking sabers...");
    auto* leftSaber = CRASH_UNLESS(il2cpp_utils::GetFieldValue(self, "_leftSaber"));
    auto* rightSaber = CRASH_UNLESS(il2cpp_utils::GetFieldValue(self, "_rightSaber"));
    CheckSaber(leftSaber, EnsureAudioSource(leftSaber, &cachedLeftSaber), clip);
    CheckSaber(rightSaber, EnsureAudioSource(rightSaber, &cachedRightSaber), clip);
    logger().debug("Complete!");
}

MAKE_HOOK_OFFSETLESS(MainMenuViewController_DidActivate, void, Il2CppObject* self, bool firstActivation, int activationType) {
    MainMenuViewController_DidActivate(self, firstActivation, activationType);
    manager.Reload();
    ReloadAlertText(self);
    if (!manager.LoadAllAudio()) {
        logger().warning("One or more AudioClips failed to load!");
        SetText("<color=#FF000000>SwingSounds:\nOne or more AudioClips failed to load! Ensure your paths are correct!</color>");
    } else {
        logger().info("All AudioClips loaded!");
        SetText("<color=#00FF0000>SwingSounds:\nAll AudioClips successfully loaded!</color>");
    }
}

static ModInfo modInfo;

Configuration& getConfig() {
    static Configuration conf(modInfo);
    return conf;
}
const Logger& logger() {
    static const Logger logg(modInfo);
    return logg;
}

extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;
}

// This function is called once il2cpp_init has been called, so il2cpp_utils and il2cpp_functions can be used safely here.
// This is where OFFSETLESS hooks must be installed.
extern "C" void load() {
    logger().info("Installing hooks...");
    INSTALL_HOOK_OFFSETLESS(SaberManager_Update, il2cpp_utils::FindMethod("", "SaberManager", "Update"));
    INSTALL_HOOK_OFFSETLESS(MainMenuViewController_DidActivate, il2cpp_utils::FindMethodUnsafe("", "MainMenuViewController", "DidActivate", 2));
    manager.Initialize();
    InitializeAlertText();
    logger().info("Installed all hooks!");
}