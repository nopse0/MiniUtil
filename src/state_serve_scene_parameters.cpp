#include "state_all.h"
#include "papyrus_utils.h"

// Communication with the Papyrus sattelite script

namespace fsm {

	std::optional<fsm_state> state_serve_scene_parameters::handle_tick(game::context& a_ctx) {
		logger_custom::trace("state_serve_scene_parameters::handle_tick");
		if (a_ctx._clock.get_play_second() >= _timeout_p) {
			logger_custom::debug("state_serve_scene_parameters::handle_tick");
			if (on_error.first) {
				logger_custom::debug("state_serve_scene_parameters::handle_tick timeout, calling error callback");
				// Forward whatever next state the lambda chooses back up to the FSM
				return on_error.second(a_ctx);
			}
		}
		return std::nullopt;
	}

	std::pair<bool, std::optional<fsm_state>> state_serve_scene_parameters::handle_get_scene_parameters(game::context& ctx, RE::TESQuest* a_this, float a_handle) {
		logger_custom::debug("state_serve_scene_parameters::get_scene_parameters");

		if (!a_this || a_handle != _handle) {
			logger_custom::debug("state_serve_scene_parameters error a_this={} a_handle={}", (void*)a_this, a_handle);
			return std::pair(false, on_error.first ? on_error.second(ctx) : std::nullopt);
		}

		auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		release_assert(vm != nullptr);

		auto policy = vm->GetObjectHandlePolicy();
		RE::VMHandle handle = policy->GetHandleForObject(a_this->GetFormType(), a_this);
		if (handle == policy->EmptyHandle()) {
			logger_custom::debug("state_serve_scene_parameters error vm handle={}", handle);
			return std::pair(false, on_error.first ? on_error.second(ctx) : std::nullopt);
		}
		RE::BSTSmartPointer<RE::BSScript::Object> scriptObject;
		if (!vm->FindBoundObject(handle, "MiniMolestMain_Script", scriptObject) || !scriptObject) {
			logger_custom::debug("state_serve_scene_parameters error scriptObject=", (void*)scriptObject.get());
			return std::pair(false, on_error.first ? on_error.second(ctx) : std::nullopt);
		}

		logger_custom::debug("state_serve_scene_parameters Found script {}", "MiniMolestMain_Script");

		int success_full = 0;

		RE::BSTSmartPointer<RE::BSScript::Array> papyrus_actors;
		if (papyrus_utils::actor_array_to_papyrus(vm, policy, _actors, papyrus_actors)) {
			logger_custom::debug("state_serve_scene_parameters Array actors converted to Papyrus");
			auto variableIter = scriptObject->GetVariable("SceneActors");
			if (variableIter) {
				success_full++;
				logger_custom::debug("Setting SceneActors size = {}", _actors.size());
				RE::BSScript::Variable var;
				var.SetArray(papyrus_actors);
				*variableIter = var;
			}
		}

		RE::BSTSmartPointer<RE::BSScript::Array> papyrus_submissive_actors;
		if (papyrus_utils::actor_array_to_papyrus(vm, policy, _submissives, papyrus_submissive_actors)) {
			logger_custom::debug("Array submissive_actors converted to Papyrus");
			auto variableIter = scriptObject->GetVariable("SceneSubmissiveActors");
			if (variableIter) {
				success_full++;
				logger_custom::debug("Setting SceneSubmissiveActors submissive_actors size = {}", _submissives.size());
				RE::BSScript::Variable var;
				var.SetArray(papyrus_submissive_actors);
				*variableIter = var;
			}
		}

		if (success_full == 2)
			return std::pair(true, on_success.first ? on_success.second(ctx) : std::nullopt);
		else
			return std::pair(false, on_error.first ? on_error.second(ctx) : std::nullopt);
	}

	std::optional<fsm_state> state_wait_scene_started::handle_tick(game::context& a_ctx) {
		logger_custom::trace("state_wait_scene_started::handle_tick");
		if (a_ctx._clock.get_play_second() >= _timeout_p) {
			if (on_error.first) {
				logger_custom::debug("state_wait_scene_started::handle_tick timeout, calling error callback");
				// Forward whatever next state the lambda chooses back up to the FSM
				return on_error.second(a_ctx);
			}
		}
		return std::nullopt;
	}
	
	std::pair<bool, std::optional<fsm_state>> state_wait_scene_started::handle_set_scene_thread_id(game::context& ctx, float a_handle, uint32_t a_thread_id) {
		logger_custom::debug("state_wait_scene_started::handle_set_scene_thread_id id={}", a_thread_id);
		if (a_handle != _handle) {
			logger_custom::debug("state_wait_scene_started::handle_set_scene_thread_id error a_handle={}", a_handle);
			return std::pair(false, on_error.first ? on_error.second(ctx) : std::nullopt);
		}
		return std::pair(true, on_success.first ? on_success.second(ctx) : std::nullopt);
	}

	std::pair<bool, std::optional<fsm_state>> state_wait_scene_started::handle_set_scene_error(game::context& ctx, float a_handle) {
		logger_custom::debug("state_wait_scene_started::handle_set_scene_error");
		if (a_handle != _handle) {
			logger_custom::debug("state_wait_scene_started::handle_set_scene_error error a_handle={}", a_handle);
			return std::pair(false, on_error.first ? on_error.second(ctx) : std::nullopt);
		}
		return std::pair(false, on_error.first ? on_error.second(ctx) : std::nullopt);
	}

}
