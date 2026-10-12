namespace papyrus_utils {

	bool actor_array_to_papyrus(RE::BSScript::Internal::VirtualMachine* vm, RE::BSScript::IObjectHandlePolicy* policy, std::vector<RE::Actor*>& actors,
		RE::BSTSmartPointer<RE::BSScript::Array>& papyrusArray)
	{
		RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> actorTypeInfoPtr;
		if (!vm->GetScriptObjectType("Actor", actorTypeInfoPtr) || !actorTypeInfoPtr) {
			logger::error("Konnte ObjectTypeInfo für 'Actor' nicht finden!");
			return false;
		}

		RE::BSScript::TypeInfo actorTypeInfo;
		actorTypeInfo.SetType(static_cast<RE::BSScript::TypeInfo::RawType>(actorTypeInfoPtr->GetRawType()));

		if (!vm->CreateArray(actorTypeInfo, static_cast<std::uint32_t>(actors.size()), papyrusArray) || !papyrusArray) {
			return false;
		}

		for (std::size_t i = 0; i < actors.size(); ++i) {
			RE::BSScript::Variable& elementVar = (*papyrusArray)[i];

			if (actors[i]) {
				RE::VMHandle actorHandle = policy->GetHandleForObject(RE::FormType::ActorCharacter, actors[i]);

				RE::BSTSmartPointer<RE::BSScript::Object> actorObject;
				if (vm->FindBoundObject(actorHandle, "Actor", actorObject) && actorObject) {
					elementVar.SetObject(actorObject);
				}
				else {
					elementVar.SetNone();
				}
			}
			else {
				elementVar.SetNone();
			}
		}

		return true;
	}



	bool force_to_alias(RE::TESQuest* a_quest, uint32_t a_aliasID, RE::Actor* a_actor)
	{
		if (!a_quest || !a_actor) {
			return false;
		}

		// 1. Iterate through the quest's aliases to find the matching ID
		for (auto* alias : a_quest->aliases) {
			if (alias && alias->aliasID == a_aliasID) {

				// 2. Cast the generic BSScript::Alias to a ReferenceAlias (REFR_RP_ALIAS)
				auto* refAlias = skyrim_cast<RE::BGSRefAlias*>(alias);
				if (refAlias) {
					// alias is already set, this shouldn't happen, if everything works correctly
					if (refAlias->GetReference())
						return false;
					// 3. Cast the Actor to a TESObjectREFR (ObjectReference)
					auto* objectRef = skyrim_cast<RE::TESObjectREFR*>(a_actor);
					if (objectRef) {

						// 4. Force the reference into the alias
						refAlias->ForceRefTo(objectRef);
						return true;
					}
				}
			}
		}

		return false;
	}

	bool clear_alias(RE::TESQuest* a_quest, uint32_t a_aliasID)
	{
		if (!a_quest) {
			return false;
		}

		// 1. Iterate through the quest's aliases to find the matching ID
		for (auto* alias : a_quest->aliases) {
			if (alias && alias->aliasID == a_aliasID) {

				// 2. Cast the generic BSScript::Alias to a ReferenceAlias (REFR_RP_ALIAS)
				auto* refAlias = skyrim_cast<RE::BGSRefAlias*>(alias);
				if (refAlias) {
					refAlias->ForceRefTo(nullptr);
					return true;
				}
			}
		}

		return false;
	}





}