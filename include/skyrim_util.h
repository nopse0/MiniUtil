#pragma once

namespace skyrim_util {
	bool is_playing();
    bool is_animating(RE::Actor* a_actor);
    bool is_valid_for_event(RE::Actor* a_actor);
    void move_to(
        RE::TESObjectREFR* a_this,
        const RE::ObjectRefHandle& a_targetHandle,
        RE::TESObjectCELL* a_targetCell,
        RE::TESWorldSpace* a_selfWorldSpace,
        const RE::NiPoint3& a_position,
        const RE::NiPoint3& a_rotation);
}

