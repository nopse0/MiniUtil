#include "state_all.h"
#include "papyrus_utils.h"

// Communication with the Papyrus sattelite script

namespace fsm {

	std::optional<fsm_state> state_serve_scene_parameters::handle_tick(game::context& a_ctx) {
		logger_custom::trace("state_serve_scene_parameters::handle_tick");
		if (a_ctx._clock.get_play_second() >= _timeout_p) {
			logger_custom::debug("state_serve_scene_parameters::handle_tick");
			if (on_timeout.first) {
				logger_custom::debug("state_serve_scene_parameters::handle_tick timeout, calling error callback");
				return on_timeout.second(a_ctx);
			}
		}
		return std::nullopt;
	}

	std::pair<bool, std::optional<fsm_state>> state_serve_scene_parameters::handle_get_scene_parameters(game::context& ctx, RE::TESQuest* a_this, float a_handle) {
		logger_custom::debug("state_serve_scene_parameters::get_scene_parameters");
		if (a_this && a_handle == _handle && on_serve.first) {
			logger_custom::debug("state_serve_scene_parameters::handle_get_scene_parameters calling callback");
			return on_serve.second(ctx, a_this, a_handle);
		}
		return std::make_pair(false, std::nullopt);
	}

	std::optional<fsm_state> state_wait_scene_set_thread_id::handle_tick(game::context& a_ctx) {
		logger_custom::trace("state_wait_scene_set_thread_id::handle_tick");
		if (a_ctx._clock.get_play_second() >= _timeout_p) {
			if (on_timeout.first) {
				logger_custom::debug("state_wait_scene_set_thread_id::handle_tick timeout, calling error callback");
				return on_timeout.second(a_ctx);
			}
		}
		return std::nullopt;
	}

	std::pair<bool, std::optional<fsm_state>> state_wait_scene_set_thread_id::handle_set_scene_thread_id(game::context& ctx, float a_handle, int32_t a_thread_id) {
		logger_custom::debug("state_wait_scene_set_thread_id::handle_set_scene_thread_id id={}", a_thread_id);
		if (a_handle == _handle && on_set.first) {
			logger_custom::debug("state_wait_scene_set_thread_id::handle_set_scene_thread_id calling callback setter");
			return on_set.second(ctx, a_handle, a_thread_id);
		}
		return std::make_pair(false, std::nullopt);
	}


	//std::optional<fsm_state> state_wait_scene_started::handle_tick(game::context& a_ctx) {
	//	logger_custom::trace("state_wait_scene_started::handle_tick");
	//	if (a_ctx._clock.get_play_second() >= _timeout_p) {
	//		if (on_error.first) {
	//			logger_custom::debug("state_wait_scene_started::handle_tick timeout, calling error callback");
	//			// Forward whatever next state the lambda chooses back up to the FSM
	//			return on_error.second(a_ctx);
	//		}
	//	}
	//	return std::nullopt;
	//}
	//
	//std::pair<bool, std::optional<fsm_state>> state_wait_scene_started::handle_set_scene_thread_id(game::context& ctx, float a_handle, uint32_t a_thread_id) {
	//	logger_custom::debug("state_wait_scene_started::handle_set_scene_thread_id id={}", a_thread_id);
	//	if (a_handle != _handle) {
	//		logger_custom::debug("state_wait_scene_started::handle_set_scene_thread_id error a_handle={}", a_handle);
	//		return std::pair(false, on_error.first ? on_error.second(ctx) : std::nullopt);
	//	}
	//	return std::pair(true, on_success.first ? on_success.second(ctx) : std::nullopt);
	//}

	//std::pair<bool, std::optional<fsm_state>> state_wait_scene_started::handle_set_scene_error(game::context& ctx, float a_handle) {
	//	logger_custom::debug("state_wait_scene_started::handle_set_scene_error");
	//	if (a_handle != _handle) {
	//		logger_custom::debug("state_wait_scene_started::handle_set_scene_error error a_handle={}", a_handle);
	//		return std::pair(false, on_error.first ? on_error.second(ctx) : std::nullopt);
	//	}
	//	return std::pair(false, on_error.first ? on_error.second(ctx) : std::nullopt);
	//}

}
