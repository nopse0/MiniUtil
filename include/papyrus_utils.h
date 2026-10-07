#pragma once

namespace papyrus_utils {

	bool actor_array_to_papyrus(RE::BSScript::Internal::VirtualMachine* vm, RE::BSScript::IObjectHandlePolicy* policy, std::vector<RE::Actor*>& actors,
		RE::BSTSmartPointer<RE::BSScript::Array>& papyrusArray);

}