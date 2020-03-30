#include "../include/config.hpp"
#include "../include/main.hpp"
#include "../extern/beatsaber-hook/shared/utils/logging.h"
#include "../extern/beatsaber-hook/shared/customui/customui.hpp"

AudioManager manager;
CustomUI::TextObject alertText;

void InitializeAlertText() {
    alertText.fontSize = 5;
    alertText.anchoredPosition = {0, -100};
}

void SetText(std::string_view text) {
    // TODO: Cache commonly used 'text' --> Il2CppString* conversions
    if (!alertText.set(text)) {
        log(WARNING, "Not displaying text: %s", text.data());
        return;
    }
}

void ReloadAlertText(Il2CppObject* mainMenu) {
    log(INFO, "Reloading alert text!");
    alertText.parentTransform = il2cpp_utils::GetPropertyValue(mainMenu, "rectTransform");
    if (alertText.gameObj == nullptr || alertText.textMesh == nullptr) {
        if (!alertText.create()) {
            log(WARNING, "Failed to create text object! Will not display text.");
        }
    }
}

// Add an AudioSource component to the Saber (if they haven't already been added)
// Get Saber.bladeSpeed (property) and ensure it is over the threshold
// Play the sound if it is
void CheckSaber(Il2CppObject* saber, Il2CppObject* audioClip) {
    if (saber == nullptr) {
        return;
    }
    // Check saber details
    float speed;
    if (!il2cpp_utils::GetPropertyValue(&speed, saber, "bladeSpeed")) {
        log(CRITICAL, "Could not saber.get_bladeSpeed()!");
        return;
    }
    if (speed <= manager.config.speedThreshold) {
        log(DEBUG, "Speed threshold not met");
        return;
    }
    static auto audioSourceTypeObject = il2cpp_utils::GetSystemType("UnityEngine", "AudioSource");
    Il2CppObject* audioSourceComponent;
    if (!il2cpp_utils::RunMethod(&audioSourceComponent, saber, "GetComponent", audioSourceTypeObject)) {
        log(CRITICAL, "Could not GetComponent(typeof(UnityEngine.AudioSource))!");
        return;
    }
    if (audioSourceComponent == nullptr) {
        if (!il2cpp_utils::RunMethod(&audioSourceComponent, saber, "AddComponent", audioSourceTypeObject)) {
            log(CRITICAL, "Could not AddComponent(typeof(UnityEngine.AudioSource))!");
            return;
        }
        static bool falseBool = false;
        il2cpp_utils::SetPropertyValue(audioSourceComponent, "loop", &falseBool);
        il2cpp_utils::SetPropertyValue(audioSourceComponent, "playOnAwake", &falseBool);
    }
    // We know that audioSourceComponent must be non-null
    float volume = manager.config.scaleVolumeWithSpeed ? speed / 100.0f : manager.config.volume;
    if (!il2cpp_utils::RunMethod(audioSourceComponent, "PlayOneShot", audioClip, volume)) {
        log(CRITICAL, "Could not audioSourceComponent.PlayOneShot(audioClip, volume)!");
        return;
    }
}

MAKE_HOOK_OFFSETLESS(SaberManager_Update, void, Il2CppObject* self) {
    SaberManager_Update(self);    
    log(DEBUG, "Getting next audio clip");
    auto clip = manager.GetNextAudioClip();
    log(DEBUG, "Got audio clip!");
    log(DEBUG, "Checking sabers...");
    CheckSaber(il2cpp_utils::GetFieldValue(self, "_leftSaber"), clip);
    CheckSaber(il2cpp_utils::GetFieldValue(self, "_rightSaber"), clip);
    log(DEBUG, "Complete!");
}

MAKE_HOOK_OFFSETLESS(MainMenuViewController_DidActivate, void, Il2CppObject* self, bool firstActivation, int activationType) {
    MainMenuViewController_DidActivate(self, firstActivation, activationType);
    manager.Reload();
    ReloadAlertText(self);
    if (!manager.LoadAllAudio()) {
        log(WARNING, "One or more AudioClips failed to load!");
        SetText("<color=#FF000000>SwingSounds:\nOne or more AudioClips failed to load! Ensure your paths are correct!</color>");
    } else {
        log(INFO, "All AudioClips loaded!");
        SetText("<color=#00FF0000>SwingSounds:\nAll AudioClips successfully loaded!</color>");
    }
}

// This function is called once il2cpp_init has been called, so il2cpp_utils and il2cpp_functions can be used safely here.
// This is where OFFSETLESS hooks must be installed.
extern "C" void load() {
    log(INFO, "Installing hooks...");
    INSTALL_HOOK_OFFSETLESS(SaberManager_Update, il2cpp_utils::FindMethod("", "SaberManager", "Update"));
    INSTALL_HOOK_OFFSETLESS(MainMenuViewController_DidActivate, il2cpp_utils::FindMethodUnsafe("", "MainMenuViewController", "DidActivate", 2));
    manager.Initialize();
    InitializeAlertText();
    log(INFO, "Installed all hooks!");
}