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

}
