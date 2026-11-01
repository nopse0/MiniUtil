#include "form_cache.h"

namespace skyrim_util {

     void reset_skills() {
        auto player = RE::PlayerCharacter::GetSingleton();
        if (player) {
            std::vector<RE::ActorValue> skills = {
                RE::ActorValue::kOneHanded, RE::ActorValue::kTwoHanded, RE::ActorValue::kArchery,
                RE::ActorValue::kHeavyArmor, RE::ActorValue::kBlock, RE::ActorValue::kSmithing,
                RE::ActorValue::kAlteration, RE::ActorValue::kConjuration, RE::ActorValue::kDestruction,
                RE::ActorValue::kIllusion, RE::ActorValue::kRestoration, RE::ActorValue::kEnchanting,
                RE::ActorValue::kLightArmor, RE::ActorValue::kSneak, RE::ActorValue::kLockpicking,
                RE::ActorValue::kPickpocket, RE::ActorValue::kSpeech, RE::ActorValue::kAlchemy
            };
            //auto avOwner = static_cast<RE::ActorValueOwner*>(player);
			auto avOwner = player->AsActorValueOwner();
            for (auto av : skills) {
                logger::info("avOwner = {}, av = {}", (void*)avOwner, av);
                avOwner->SetBaseActorValue(av, 1.0f);
                avOwner->SetActorValue(av, 1.0f);
            }
        }
    }

    bool is_playing()
    {
        auto ui = RE::UI::GetSingleton();
        if (!ui) {
            return false;
        }

        // 1. Block if the game is natively paused (Esc menu, inventory, etc.)
        if (ui->GameIsPaused()) {
            return false;
        }

        // 2. Block if an "Application Menu" is open. 
        // This internal engine flag covers the Main Menu (title screen) and Loading screens.
        if (ui->IsApplicationMenuOpen()) {
            return false;
        }

        // 3. SkyUI / MCM Specific Protection (Crucial for Skyrim Souls RE compatibility)
        // The journal menu includes the MCM. Since it might be unpaused by mods, 
        // we explicitly check its presence on the stack.
        if (ui->IsMenuOpen("Journal Menu")) {
            return false;
        }

        // 4. (Optional) Prevent running if the console or full-screen menus are open
        if (ui->IsMenuOpen("Console") || ui->IsMenuOpen("Loading Menu")) {
            return false;
        }

        return true;
    }


    // Private function of TESObjectReference, or so, I think
    void move_to(
        RE::TESObjectREFR* a_this,
        const RE::ObjectRefHandle& a_targetHandle,
        RE::TESObjectCELL* a_targetCell,
        RE::TESWorldSpace* a_selfWorldSpace,
        const RE::NiPoint3& a_position,
        const RE::NiPoint3& a_rotation)
    {
        if (!a_this) return;

        // Wir definieren den Funktions-Typen genau wie in der CommonLib
        using func_t = void(RE::TESObjectREFR*, const RE::ObjectRefHandle&, RE::TESObjectCELL*, RE::TESWorldSpace*, const RE::NiPoint3&, const RE::NiPoint3&);

        // Nutzung der plattformunabhängigen IDs von CommonLibSSE-NG
        static REL::Relocation<func_t> func{ RELOCATION_ID(56227, 56626) };

        func(a_this, a_targetHandle, a_targetCell, a_selfWorldSpace, a_position, a_rotation);
    }

    bool is_animating(RE::Actor* a_actor) {
        if (!a_actor) return false;

        if (form_cache::SexLabAnimatingFaction && a_actor->IsInFaction(form_cache::SexLabAnimatingFaction)) {
            return true;
        }

        return false;
    }

    bool is_valid_for_event(RE::Actor* a_actor) {
        if (!a_actor || a_actor->IsDead() || !a_actor->Is3DLoaded()) {
            return false;
        }

        return !is_animating(a_actor);
    }


}
