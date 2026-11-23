#include "state_all.h"
#include "papyrus_utils.h"

// Communication with the Papyrus sattelite script

namespace fsm {

	std::optional<fsm_state> state_scene_start::handle_tick(game::context& a_ctx) {
		logger_custom::trace("state_scene_start::handle_tick");
		if (a_ctx._clock.get_play_second() >= _timeout_p) {
			logger_custom::debug("state_scene_start::handle_tick");
			if (fn_timeout.first) {
				logger_custom::debug("state_scene_start::handle_tick timeout, calling error callback");
				return fn_timeout.second(a_ctx);
			}
		}
		return std::nullopt;
	}

	std::pair<bool, std::optional<fsm_state>> state_scene_start::handle_get_scene_parameters(game::context& ctx, RE::TESQuest* a_this, float a_handle) {
		logger_custom::debug("state_scene_start::get_scene_parameters");
		if (a_this && a_handle == _handle && fn_papyrus.first) {
			logger_custom::debug("state_scene_start::handle_get_scene_parameters calling callback");
			return fn_papyrus.second(ctx, a_this, a_handle);
		}
		return std::make_pair(false, std::nullopt);
	}

	//std::optional<fsm_state> state_wait_scene_set_thread_id::handle_tick(game::context& a_ctx) {
	//	logger_custom::trace("state_wait_scene_set_thread_id::handle_tick");
	//	if (a_ctx._clock.get_play_second() >= _timeout_p) {
	//		if (fn_timeout.first) {
	//			logger_custom::debug("state_wait_scene_set_thread_id::handle_tick timeout, calling error callback");
	//			return fn_timeout.second(a_ctx);
	//		}
	//	}
	//	return std::nullopt;
	//}

	//std::pair<bool, std::optional<fsm_state>> state_wait_scene_set_thread_id::handle_set_scene_thread_id(game::context& ctx, float a_handle, int32_t a_thread_id) {
	//	logger_custom::debug("state_wait_scene_set_thread_id::handle_set_scene_thread_id id={}", a_thread_id);
	//	if (a_handle == _handle && on_set.first) {
	//		logger_custom::debug("state_wait_scene_set_thread_id::handle_set_scene_thread_id calling callback setter");
	//		return on_set.second(ctx, a_handle, a_thread_id);
	//	}
	//	return std::make_pair(false, std::nullopt);
	//}

}
