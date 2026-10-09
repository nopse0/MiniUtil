#include "minigame.h"
#include "form_cache.h"
#include "actor_util.h"
#include "controller.h"
#include "sl_scenes.h"
#include "random.h"

namespace main_loop {
	void install();
}

#if 0
namespace scanner {
	bool scan_actors(float a_radius, size_t a_nearest_males, size_t a_nearest_females,
		std::vector<RE::Actor*>& a_male_actors, std::vector<bool>& a_male_see_player,
		std::vector<RE::Actor*>& a_female_actors, std::vector<bool>& a_female_see_player);
}

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

std::string ExecuteScan_ScriptName("MiniMolestApproach_Script");

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
	if (!vm->FindBoundObject(handle, ExecuteScan_ScriptName.c_str(), scriptObject) || !scriptObject) {
		return false;
	}

	logger::debug("Found {}", ExecuteScan_ScriptName.c_str());

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

void MinigameTestCpp(RE::TESQuest* thisInstance, int duration_secs)
{
	minigame::struggle_game::get_instance().start(duration_secs);
}
#endif

void MiniUtil_OnDialogEnd(RE::StaticFunctionTag*, RE::Actor* akSpeaker)
{
	controller::controller::get_instance().on_dialog_end(akSpeaker);
}

bool MiniMolest_GetSceneParameters(RE::TESQuest* thisInstance, float handle)
{
	return sl_scenes::sl_scene::get_instance().get_scene_parameters(thisInstance, handle);
}

bool MiniUtil_SetSceneThreadID(RE::StaticFunctionTag*, float handle, uint32_t threadId)
{
	return sl_scenes::sl_scene::get_instance().set_thread_id(handle, threadId);
}


bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* vm)
{
	if (!vm) return false;

	vm->RegisterFunction("OnDialogEnd", "MiniUtil_Script", MiniUtil_OnDialogEnd);
	vm->RegisterFunction("GetSceneParameters", "MiniMolestMain_Script", MiniMolest_GetSceneParameters);
	vm->RegisterFunction("SetSceneThreadID", "MiniUtil_Script", MiniUtil_SetSceneThreadID);

	return true;
}

// ImGui
void __stdcall render() {
	minigame::struggle_game::get_instance().render();
}

bool __stdcall on_input(RE::InputEvent* a_event) {
	return minigame::struggle_game::get_instance().on_input(a_event);
}

void InitializeMCP() {
	if (!SKSEMenuFramework::IsInstalled()) {
		return;
	}

	SKSEMenuFramework::AddHudElement(render);
	SKSEMenuFramework::AddInputEvent(on_input);
}

// Skyrim Forms (to be exceuted after data loaded)
void InitializeForms() {
	auto dataHandler = RE::TESDataHandler::GetSingleton();
	if (!dataHandler) return;

	const char* modName = "MiniMolest.esp";

	form_cache::MiniMolestForceGreetOutcome = dataHandler->LookupForm<RE::TESGlobal>(0x00031354, modName);
	form_cache::MiniMolestForceGreetType = dataHandler->LookupForm<RE::TESGlobal>(0x00017E46, modName);
	form_cache::MiniMolestForceGreetPackage = dataHandler->LookupForm<RE::TESPackage>(0x000036CC, modName);
	form_cache::MiniMolestDoNothingPackage = dataHandler->LookupForm<RE::TESPackage>(0x0001CF49, modName);
	form_cache::MiniMolestAnimMarker = dataHandler->LookupForm<RE::TESObjectREFR>(0x0001CF48, modName);

	if (!form_cache::MiniMolestForceGreetOutcome
		|| !form_cache::MiniMolestForceGreetType
		|| !form_cache::MiniMolestForceGreetPackage
		|| !form_cache::MiniMolestDoNothingPackage
		|| !form_cache::MiniMolestAnimMarker) {
		logger::error("Failed to load some forms from {}!", modName);
	}
	else {
		logger::info("Successfully loaded forms from {}.", modName);
	}
}


void InitializeSerialization()
{
	// Deine internen Subtypes (als 4-Byte-Integers deklariert)
	enum class module_id_t : std::uint32_t {
		RandomGenerator = 'RNDM',
		Controller = 'CTRL',
	};


	auto serialization = SKSE::GetSerializationInterface();
	if (!serialization) {
		logger::error("Serialization Interface konnte nicht geladen werden!");
		return;
	}

	serialization->SetRevertCallback([](SKSE::SerializationInterface*) {
		logger::debug("SKSE::RevertCallback");
		random::random::get_instance().reset();
		sl_scenes::sl_scene::get_instance().reset();
		controller::controller::get_instance().reset();
	});

	serialization->SetSaveCallback([](SKSE::SerializationInterface* a_serde) {
		logger::debug("SKSE::SaveCallback");
		if (!a_serde->OpenRecord('MMST', 1)) {
			logger::error("Konnte Haupt-Record nicht öffnen!");
			return;
		}

		auto module = module_id_t::RandomGenerator;
		a_serde->WriteRecordData(&module, sizeof(module));
		random::random::get_instance().serialize(a_serde);
	});

	serialization->SetLoadCallback([](SKSE::SerializationInterface* a_serde) {
		std::uint32_t type;
		std::uint32_t version;
		std::uint32_t length;

		logger::debug("SKSE::LoadCallback");
		// SKSE sucht nach unserem Haupt-Eintrag
		while (a_serde->GetNextRecordInfo(type, version, length)) {
			if (type == 'MMST' && version == 1) {

				std::uint32_t bytesRead = 0;

				// Schleife läuft, solange wir noch nicht die gesamte 'length' des Records gelesen haben
				while (bytesRead < length) {
					std::uint32_t subTypeRaw = 0;
					if (!a_serde->ReadRecordData(&subTypeRaw, sizeof(subTypeRaw))) {
						break; // Fehler oder Ende des Records erreicht
					}
					bytesRead += sizeof(subTypeRaw);

					module_id_t currentSub = static_cast<module_id_t>(subTypeRaw);

					switch (currentSub) {
					case module_id_t::RandomGenerator:
						bytesRead += random::random::get_instance().deserialize(a_serde);
						break;

					case module_id_t::Controller:
						// AnimationManager::Get().Deserialize(a_serde);
						break;

					default:
						logger::error("Unbekannter Subtype im MMST-Record gefunden! Breche Laden ab, um Korruption zu verhindern.");
						return;
					}
				}
			}
		}
		});
}

void RegisterForModEvents() {
	auto eventSource = SKSE::GetModCallbackEventSource();
	if (eventSource) {
		// Wir fügen unseren Callback hinzu.
		// Wichtig: Die Engine löscht diese Zuweisung bei jedem Ladebildschirm!
		eventSource->AddEventSink(&controller::controller::get_instance());
		logger::info("Erfolgreich für Framework Mod Events registriert.");
	}
}


// 3. Der Message Listener
void OnSKSEMessage(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kNewGame:
		random::random::get_instance().new_game();
		RegisterForModEvents();
		break;

	case SKSE::MessagingInterface::kPostLoad:
		InitializeSerialization();
		RegisterForModEvents();
		break;

	case SKSE::MessagingInterface::kDataLoaded:
		InitializeForms();
		main_loop::install();
		actor_util::internal::initialize();
		break;
	}
}


// Der SKSE Einstiegspunkt
extern "C" __declspec(dllexport) bool SKSEPlugin_Load(const SKSE::LoadInterface* a_skse) {
	SKSE::Init(a_skse);

	// 1. ALLOCATE THE TRAMPOLINE BLOCK HERE FIRST!
	//SKSE::AllocTrampoline(64);

	const char* const ini_name = "data/skse/plugins/MiniUtil.ini";
	CSimpleIniA ini;
	SI_Error err = ini.LoadFile(ini_name);
	spdlog::level::level_enum level = spdlog::level::info;
	if (!err) {
		level = static_cast<spdlog::level::level_enum>(ini.GetLongValue("Config", "LogLevel", level));
		//ExecuteScan_ScriptName = std::string(ini.GetValue("Config", "ScriptName", ExecuteScan_ScriptName.c_str()));
	}
	else {
		logger::info("Could not read config file {}", ini_name);
	}
	spdlog::set_level(level);

	auto* messaging = SKSE::GetMessagingInterface();
	if (messaging) {
		messaging->RegisterListener(OnSKSEMessage);
	}

	InitializeMCP();

	auto* papyrus = SKSE::GetPapyrusInterface();
	if (papyrus) {
		papyrus->Register(RegisterPapyrusFunctions);
	}

	return true;
}
