#include "state_idle.h"
#include "state_force_greet_1npc.h"
#include "fsm.h"
#include "controller.h"
#include "spdlog/cfg/helpers.h"
#include "minigame.h"
#include "form_cache.h"
#include "actor_util.h"
#include "sl_scenes.h"
#include "random.h"
#include "events.h"
#include "spdlog/sinks/sink.h"

namespace main_loop {
	void install();
}

void LoggingTest(std::string a_logger_name) {
	auto logger = spdlog::get(a_logger_name);
	if (!logger) {
		auto def_logger = spdlog::default_logger();
		if (def_logger) {
			logger = std::make_shared<spdlog::logger>(a_logger_name,
				def_logger->sinks().begin(),
				def_logger->sinks().end());
			/*logger->set_formatter(std::make_unique<spdlog::pattern_formatter>("[%T.%f] [%=5t] [%n] [%L] [%s:%# %!] %v"));*/
			spdlog::initialize_logger(logger);
			//spdlog::register_logger(logger);
		}
	}
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

bool MiniUtil_SetSceneError(RE::StaticFunctionTag*, float handle)
{
	return sl_scenes::sl_scene::get_instance().set_error(handle);
}

bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* vm)
{
	if (!vm) return false;

	vm->RegisterFunction("OnDialogEnd", "MiniUtil_Script", MiniUtil_OnDialogEnd);
	vm->RegisterFunction("GetSceneParameters", "MiniMolestMain_Script", MiniMolest_GetSceneParameters);
	vm->RegisterFunction("SetSceneThreadID", "MiniUtil_Script", MiniUtil_SetSceneThreadID);
	vm->RegisterFunction("SetSceneError", "MiniUtil_Script", MiniUtil_SetSceneError);

	return true;
}

// ImGui
void __stdcall render_minigame() {
	minigame::struggle_game::get_instance().render();
}

bool __stdcall on_input_minigame(RE::InputEvent* a_event) {
	return minigame::struggle_game::get_instance().on_input(a_event);
}

#if 0
void __stdcall render_ui() {
	auto& s = events::manager::get_instance();
	if (s.ui_is_loading())
		return;


	static events::settings _settings;
	if (ImGuiMCP::InputDouble("base_chance", &_settings.base_chance_global)) {
		
	}
}
#endif

void InitializeMCP() {
	if (!SKSEMenuFramework::IsInstalled()) {
		return;
	}

	SKSEMenuFramework::AddHudElement(render_minigame);
	SKSEMenuFramework::AddInputEvent(on_input_minigame);

	//SKSEMenuFramework::SetSection("MiniMolest");
	//SKSEMenuFramework::AddSectionItem("MiniMolest", render_ui);
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
	form_cache::MiniMolestForceGreetWithOneFollower = dataHandler->LookupForm<RE::TESPackage>(0x00054A62, modName);
	form_cache::MiniMolestForceGreetFollowerPackage = dataHandler->LookupForm<RE::TESPackage>(0x00054A61, modName);

	if (!form_cache::MiniMolestForceGreetOutcome
		|| !form_cache::MiniMolestForceGreetType
		|| !form_cache::MiniMolestForceGreetPackage
		|| !form_cache::MiniMolestDoNothingPackage
		|| !form_cache::MiniMolestAnimMarker
		|| !form_cache::MiniMolestForceGreetFollowerPackage
		|| !form_cache::MiniMolestForceGreetWithOneFollower
		)
	{
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

	serialization->SetUniqueID('MMST');

	serialization->SetRevertCallback([](SKSE::SerializationInterface*) {
		logger::debug("SKSE::RevertCallback");
		random::random::get_instance().reset();
		// sl_scenes::sl_scene::get_instance().revert();  // managed by controller
		controller::controller::get_instance().revert();
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

		module = module_id_t::Controller;
		a_serde->WriteRecordData(&module, sizeof(module));
		controller::controller::get_instance().serialize(a_serde);
	});

	serialization->SetLoadCallback([](SKSE::SerializationInterface* a_serde) {
		
		
		//std::thread t1
		//{
		//	[] {
		//		LoggingTest("custom");
		//	}
		//};
		//t1.join();
		
		std::uint32_t type;
		std::uint32_t version;
		std::uint32_t length;

		logger::debug("SKSE::LoadCallback");
		// SKSE sucht nach unserem Haupt-Eintrag
		while (a_serde->GetNextRecordInfo(type, version, length)) {
			if (type == 'MMST' && version == 1) {
				logger::debug("Found record info of type 'MMST', length = {}", length);

				std::uint32_t bytesRead = 0;

				bool randomGeneratorRead = false;
				bool controllerRead = false;

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
						if (randomGeneratorRead) {
							logger::error("Save game contains multiple RandomGenerator records!");
						}
						bytesRead += random::random::get_instance().deserialize(a_serde);
						randomGeneratorRead = true;
						break;

					case module_id_t::Controller:
						if (controllerRead) {
							logger::error("Save game contains multiple Controller records!");
						}
						bytesRead += controller::controller::get_instance().deserialize(a_serde);
						controllerRead = true;
						break;

					default:
						logger::error("Unbekannter Subtype im MMST-Record gefunden! Breche Laden ab, um Korruption zu verhindern.");
						return;
					}
				}
				if (bytesRead != length || randomGeneratorRead == false || controllerRead == false) {
					logger::error("Save game corruption! bytesRead={},  randomGeneratorRead={}, controllerRead={}", 
						bytesRead, randomGeneratorRead, controllerRead);
				}
				logger::info("Record 'MMST' read from save game.");
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

//namespace logger_custom {
//	MINIUTILS_MAKE_CUSTOM_LOGGER(trace, "custom", spdlog::level::trace);
//}

// Der SKSE Einstiegspunkt
extern "C" __declspec(dllexport) bool SKSEPlugin_Load(const SKSE::LoadInterface* a_skse) {
	SKSE::Init(a_skse);

	//===========================================================================

	const char* const ini_name = "data/skse/plugins/MiniUtil.ini";
	CSimpleIniA ini;
	SI_Error err = ini.LoadFile(ini_name);

	spdlog::level::level_enum global_level = spdlog::level::info;
	std::string cfg_string = "";

	if (err == SI_OK) {
		const char* global_level_str = ini.GetValue("Config", "LogLevel", "info");
		logger::info("global_level_str = {}", global_level_str);
		cfg_string = global_level_str;

		const char* default_pattern = "[%T.%f] [%=5t] [%n] [%L] %v";
		const char* custom_pattern = ini.GetValue("Config", "LogPatternFormat", default_pattern);

		//if (auto def_logger = spdlog::default_logger()) {
		//	for (auto& sink : def_logger->sinks()) {
		//		sink->set_pattern(custom_pattern);
		//	}
		//}
		spdlog::set_pattern(custom_pattern);

		// 2. Alle Custom-Logger aus der Sektion [LogLevels] auslesen
		CSimpleIniA::TNamesDepend keys;
		if (ini.GetAllKeys("LogLevels", keys)) {
			for (const auto& key : keys) {
				const char* custom_level = ini.GetValue("LogLevels", key.pItem, "info");
				// Format anfügen: ,logger_name=level
				cfg_string += std::format(",{}={}", key.pItem, custom_level);
			}
		}
	}
	else {
		logger::info("Could not read config file {}", ini_name);
		cfg_string = "info"; // Fallback

		// Fallback-Pattern für den Fehlerfall
		if (auto def_logger = spdlog::default_logger()) {
			for (auto& sink : def_logger->sinks()) {
				sink->set_pattern("[%T.%f] [%n] [%L] %v");
			}
		}
	}

	// 3. Magierunde: spdlog konfigurieren!
	// Übergibt z.B.: "info,event_chain=debug,network=err"
	logger::info("cfg_string = {}", cfg_string);
	spdlog::cfg::helpers::load_levels(cfg_string);



	/*LoggingTest("custom");
	//logger_test::debug("Custom logger test #1");

	auto logger = spdlog::get("custom");
	if (logger) {
		logger::info("custom logger is defined");
		//logger->set_level(spdlog::level::trace);
		//logger->set_formatter(std::make_unique<spdlog::pattern_formatter>("[%T.%f] [%=5t] [%n] [%L] [%s:%# %!] %v"));
	}
	else
		logger::info("custom logger is undefined");
	
	logger->debug("Custom logger test #2");*/

	//===============================================================================

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



