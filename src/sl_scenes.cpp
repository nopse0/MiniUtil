#include "papyrus_utils.h"
#include "random.h"
#include "sl_scenes.h"

namespace sl_scenes {

	void sl_scene::revert()
	{
		logger::debug("sl_scene::reset");
		_handle = random::random::get_instance().gen_float(1.f, 10000.f);
		_actors.clear();
		_submissive_actors.clear();
		_sl_thread_id = 0;
		_error = false;
	}

	bool sl_scene::set_thread_id(float a_handle, uint32_t a_sl_thread_id)
	{
		logger::debug("sl_scene::set_thread_id");

		if (a_handle != _handle)
			return false;
		_sl_thread_id = a_sl_thread_id;
		logger::debug("sl_thread_id = {}", _sl_thread_id);
		return true;
	}

	bool sl_scene::set_error(float a_handle)
	{
		logger::debug("sl_scene::set_error");

		if (a_handle != _handle)
			return false;
		_error = true;
		return true;
	}

	bool sl_scene::get_scene_parameters(RE::TESQuest* a_this, float a_handle)
	{
		logger::info("sl_scene::get_scene_parameters");

		if (!a_this)
			return false;

		if (a_handle != _handle)
			return false;

		auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		if (!vm)
			return false;

		auto policy = vm->GetObjectHandlePolicy();
		RE::VMHandle handle = policy->GetHandleForObject(a_this->GetFormType(), a_this);
		if (handle == policy->EmptyHandle())
			return false;

		RE::BSTSmartPointer<RE::BSScript::Object> scriptObject;
		if (!vm->FindBoundObject(handle, "MiniMolestMain_Script", scriptObject) || !scriptObject) {
			return false;
		}

		logger::debug("Found script {}", "MiniMolestMain_Script");

		RE::BSTSmartPointer<RE::BSScript::Array> papyrus_actors;
		if (papyrus_utils::actor_array_to_papyrus(vm, policy, _actors, papyrus_actors)) {
			logger::debug("Array actors converted to Papyrus");
			auto variableIter = scriptObject->GetVariable("SceneActors");
			if (variableIter) {
				logger::debug("Setting SceneActors");
				logger::debug("SlScene actors size = {}", _actors.size());
				RE::BSScript::Variable var;
				var.SetArray(papyrus_actors);
				*variableIter = var;
			}
		}

		RE::BSTSmartPointer<RE::BSScript::Array> papyrus_submissive_actors;
		if (papyrus_utils::actor_array_to_papyrus(vm, policy, _submissive_actors, papyrus_submissive_actors)) {
			logger::debug("Array submissive_actors converted to Papyrus");
			auto variableIter = scriptObject->GetVariable("SceneSubmissiveActors");
			if (variableIter) {
				logger::debug("Setting SceneSubmissiveActors");
				logger::debug("SlScene submissive_actors size = {}", _submissive_actors.size());
				RE::BSScript::Variable var;
				var.SetArray(papyrus_submissive_actors);
				*variableIter = var;
			}
		}

		return true;
	}

	bool sl_scene::trigger_scene(std::vector<RE::Actor*> a_actors, std::vector<RE::Actor*> a_submissive_actors, const std::string& a_hook_postfix)
	{
		logger::debug("sl_scene::trigger_scene");

		_handle++;
		_sl_thread_id = 0;
		_error = false;
		_actors = a_actors;
		_submissive_actors = a_submissive_actors;

		SKSE::ModCallbackEvent modEvent;
		modEvent.eventName = RE::BSFixedString("MiniUtilStartScene");
		modEvent.strArg = a_hook_postfix;
		modEvent.numArg = _handle;
		modEvent.sender = nullptr;

		if (auto inputSrc = SKSE::GetModCallbackEventSource()) {
			inputSrc->SendEvent(&modEvent);
			return true;
		}

		return false;
	}

}
