#pragma once

namespace papyrus_utils {

	template <typename T>
	[[nodiscard]] T* lookup(const std::string_view& a_editorID)
	{
		auto* form = RE::TESForm::LookupByEditorID(a_editorID);
		return form ? form->As<T>() : nullptr;
	}

	bool actor_array_to_papyrus(RE::BSScript::Internal::VirtualMachine* vm, RE::BSScript::IObjectHandlePolicy* policy, std::vector<RE::Actor*>& actors,
		RE::BSTSmartPointer<RE::BSScript::Array>& papyrusArray);

	bool force_to_alias(RE::TESQuest* a_quest, uint32_t a_aliasID, RE::Actor* a_actor);
	bool clear_alias(RE::TESQuest* a_quest, uint32_t a_aliasID);
}
