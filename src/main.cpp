#include "REL/Relocation.h"
#include "SKSE/SKSE.h"

namespace scanner {
	bool scan_actors(float a_radius, size_t a_nearest_males, size_t a_nearest_females,
		std::vector<RE::Actor*>& a_male_actors, std::vector<bool>& a_male_see_player,
		std::vector<RE::Actor*>& a_female_actors, std::vector<bool>& a_female_see_player);
}

bool actor_array_to_papyrus(RE::BSScript::Internal::VirtualMachine* vm, RE::BSScript::IObjectHandlePolicy* policy, std::vector<RE::Actor*>& actors,
	RE::BSTSmartPointer<RE::BSScript::Array>& papyrusArray)
{
	RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> actorTypeInfoPtr;
	// Falls GetScriptObjectType eine Fehlermeldung wirft, probiere alternativ: vm->GetTypeInfo("Actor", actorTypeInfoPtr)
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

		//(*papyrusArray)[i] = actorVar;
	}

	return true;
}

bool bool_array_to_papyrus(
	RE::BSScript::Internal::VirtualMachine* vm,
	RE::BSScript::IObjectHandlePolicy* policy,
	const std::vector<bool>& booleans,  // Per Referenz übergeben, um Kopien zu vermeiden
	RE::BSTSmartPointer<RE::BSScript::Array>& papyrusArray)
{
	RE::BSScript::TypeInfo boolTypeInfo;
	boolTypeInfo.SetType(RE::BSScript::TypeInfo::RawType::kBool);

	// Array über die VM erstellen
	if (!vm->CreateArray(boolTypeInfo, static_cast<std::uint32_t>(booleans.size()), papyrusArray) || !papyrusArray) {
		return false;
	}

	// Elemente direkt befüllen
	for (std::size_t i = 0; i < booleans.size(); ++i) {
		// 1. Sicheres Auslesen aus dem optimierten std::vector<bool>
		bool actualValue = booleans[i];

		// 2. Direkt die Referenz des Elements im Papyrus-Array holen und beschreiben
		RE::BSScript::Variable& elementVar = (*papyrusArray)[i];
		elementVar.SetBool(actualValue);
	}

	return true;
}


bool ExecuteScan(
	RE::TESQuest* thisInstance,
	float a_radius, int a_nearest_males, int a_nearest_females)
{
	logger::info("ExecuteScan");

	if (!thisInstance)
		return false;

	auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
	if (!vm)
		return false;

	auto policy = vm->GetObjectHandlePolicy();
	RE::VMHandle handle = policy->GetHandleForObject(thisInstance->GetFormType(), thisInstance);
	if (handle == policy->EmptyHandle())
		return false;

	RE::BSTSmartPointer<RE::BSScript::Object> scriptObject;
	if (!vm->FindBoundObject(handle, "MiniMolestApproach_Script", scriptObject) || !scriptObject) {
		return false;
	}

	logger::debug("Found MiniMolestApproach_Script");

	std::vector<RE::Actor*> male_actors;
	std::vector<bool>       male_see_player;
	std::vector<RE::Actor*> female_actors;
	std::vector<bool>       female_see_player;

	auto result = scanner::scan_actors(a_radius, a_nearest_males, a_nearest_females, male_actors, male_see_player,
		female_actors, female_see_player);
	logger::debug("scan_actors: result = {}", result);
	if (!result)
		return false;

	RE::BSTSmartPointer<RE::BSScript::Array> papyrus_male_actors;
	if (actor_array_to_papyrus(vm, policy, male_actors, papyrus_male_actors)) {
		logger::debug("Male actors array converted to Papyrus");
		auto variableIter = scriptObject->GetVariable("MaleActors");
		if (variableIter) {
			logger::debug("Setting MaleActors_var");
			logger::debug("Male actors size = {}", male_actors.size());
			if (male_actors.size())
				logger::debug("Male actors[0] = {}", (void*)(male_actors[0]));
			RE::BSScript::Variable var;
			var.SetArray(papyrus_male_actors);
			*variableIter = var;
		}
	}
	RE::BSTSmartPointer<RE::BSScript::Array> papyrus_female_actors;
	if (actor_array_to_papyrus(vm, policy, female_actors, papyrus_female_actors)) {
		auto variableIter = scriptObject->GetVariable("FemaleActors");
		if (variableIter) {
			logger::debug("Setting FemaleActors_var");
			RE::BSScript::Variable var;
			var.SetArray(papyrus_female_actors);
			*variableIter = var;
		}
	}
	RE::BSTSmartPointer<RE::BSScript::Array> papyrus_male_see_player;
	if (bool_array_to_papyrus(vm, policy, male_see_player, papyrus_male_see_player)) {
		auto variableIter = scriptObject->GetVariable("MaleSeePlayer");
		if (variableIter) {
			logger::debug("Setting MaleSeePlayer_var");
			RE::BSScript::Variable var;
			var.SetArray(papyrus_male_see_player);
			*variableIter = var;
		}
	}
	RE::BSTSmartPointer<RE::BSScript::Array> papyrus_female_see_player;
	if (bool_array_to_papyrus(vm, policy, female_see_player, papyrus_female_see_player)) {
		auto variableIter = scriptObject->GetVariable("FemaleSeePlayer");
		if (variableIter) {
			logger::debug("Setting FemaleSeePlayer_var");
			RE::BSScript::Variable var;
			var.SetArray(papyrus_female_see_player);
			*variableIter = var;
		}
	}

	return true;
}

bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* vm)
{
	if (!vm) return false;

	// "ExecuteScan" wird an das Instanz-Skript "MiniMolestApproach_Script" gebunden
	vm->RegisterFunction("ExecuteScan", "MiniMolestApproach_Script", ExecuteScan);

	SKSE::log::info("ExecuteScan erfolgreich an MiniMolestApproach_Script gebunden.");
	return true;
}

// 3. Der Message Listener
void OnSKSEMessage(SKSE::MessagingInterface::Message* a_msg)
{
	if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) {
		auto* papyrus = SKSE::GetPapyrusInterface();
		if (papyrus) {
			// HIER übergeben wir jetzt direkt die saubere Funktion ohne verschachteltes Lambda
			papyrus->Register(RegisterPapyrusFunctions);
		}
	}
}

// Der SKSE Einstiegspunkt
extern "C" __declspec(dllexport) bool SKSEPlugin_Load(const SKSE::LoadInterface* a_skse) {
	SKSE::Init(a_skse);

	auto* messaging = SKSE::GetMessagingInterface();
	if (messaging) {
		messaging->RegisterListener(OnSKSEMessage);
	}

	return true;
}
