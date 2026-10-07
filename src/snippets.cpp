#include "RE/Skyrim.h"

bool IsActorInAnyQuestAlias(RE::Actor* a_actor)
{
    if (!a_actor) {
        return false;
    }

    auto dataHandler = RE::TESDataHandler::GetSingleton();
    if (!dataHandler) {
        return false;
    }

    for (auto* quest : dataHandler->GetFormArray<RE::TESQuest>()) {
        if (!quest || !quest->IsRunning()) {
            continue;
        }

        for (auto* baseAlias : quest->aliases) {
            if (!baseAlias) {
                continue;
            }

            auto* refAlias = skyrim_cast<RE::BGSRefAlias*>(baseAlias);
            if (!refAlias) {
                continue;
            }

            //if (refAlias->IsOptional()) {
            //    continue;
            //}

            // Directly call the member function exposed in your class header
            RE::Actor* filledActor = refAlias->GetActorReference();

            if (filledActor == a_actor) {
                return true; // Match found!
            }
        }
    }

    return false;
}


// Struct to hold the result details
struct ActiveBehaviorInfo {
    RE::TESPackage* activePackage = nullptr;
    RE::TESQuest* controllingQuest = nullptr; // Will be null if the package is from Actor Base/Template
    bool            isControlledByQuest = false;
};


ActiveBehaviorInfo GetActorCurrentControllingQuest(RE::Actor* a_actor)
{
    ActiveBehaviorInfo info;

    if (!a_actor) {
        return info;
    }

    // 1. Safe layout resolution via CommonLibSSE-NG's GetRuntimeData()
    auto& runtimeData = a_actor->GetActorRuntimeData();
    auto* aiProcess = runtimeData.currentProcess;

    if (!aiProcess) {
        return info;
    }

    // 2. Fetch the active runtime AI package
    RE::TESPackage* currentPackage = aiProcess->currentPackage.package;
    if (!currentPackage) {
        return info;
    }

    info.activePackage = currentPackage;

    // 3. Directly extract the quest pointer using the exact 'ownerQuest' member
    if (currentPackage->ownerQuest) {
        info.controllingQuest = currentPackage->ownerQuest;
        info.isControlledByQuest = true;
    }

    return info;
}


//#include "RE/Skyrim.h"
//#include <string_view>
#include <unordered_set>

// Eine beispielhafte Blacklist für Quests, die NIEMALS unterbrochen werden dürfen
const std::unordered_set<std::string_view> criticalQuestBlacklist = {
    "DialogueGeneric",      // Wichtig für laufende Standard-Dialoge
    "MS01",                 // Beispiel für eine wichtige Vanilla-Quest
    "3DNPC_MarriageQuest"   // Beispiel für eine bekannte Mod-Quest (Interesting NPCs)
};

bool IsQuestBlacklisted(RE::TESQuest* a_quest)
{
    if (!a_quest) return false;

    // Holt den EditorID-Namen (z.B. "DialogueGeneric")
    std::string_view editorID = a_quest->GetFormEditorID();

    if (editorID.empty()) {
        return false;
    }

    // Prüfen, ob die Quest in unserer Verbotsliste steht
    return criticalQuestBlacklist.contains(editorID);
}

bool CanSafeInteractWithActor(RE::Actor* a_actor)
{
    // Schritt 1: Steuert eine Quest AKTUELL das physische Verhalten des NPCs?
    ActiveBehaviorInfo behavior = GetActorCurrentControllingQuest(a_actor);
    if (behavior.isControlledByQuest && behavior.controllingQuest) {

        // Wenn die steuernde Quest eine hohe Priorität hat oder auf der Blacklist steht -> FINGER WEG
        if (behavior.controllingQuest->data.priority >= 60 || IsQuestBlacklisted(behavior.controllingQuest)) {
            return false;
        }
    }

    // Schritt 2: Wird der Actor passiv von einer kritischen Mod in einem Alias gehalten?
    // (Hier nutzt du deinen ersten Alias-Scan, filterst aber harmlose Mods heraus)
    if (IsActorInAnyQuestAlias(a_actor)) {
        // Hier könntest du den Alias-Scan so erweitern, dass er die Quest zurückgibt,
        // um zu prüfen, ob es sich nur um ein harmloses Hintergrund-System handelt.
    }

    return true; // NPC ist frei oder die blockierende Mod ist unkritisch!
}

