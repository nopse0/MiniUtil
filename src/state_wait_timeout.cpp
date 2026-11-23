#include "state_all.h"
#include "game_context.h"

namespace fsm {

	std::optional<fsm_state> state_sleep::handle_tick(game::context& a_ctx) {
		logger_custom::trace("state_sleep::handle_tick");
		if (a_ctx._clock.get_play_second() >= _timeout_p) {
			logger_custom::debug("state_sleep::handle_tick");
			if (fn_timeout.first) {
				logger_custom::debug("state_sleep::handle_tick calling timeout callback");
				// Forward whatever next state the lambda chooses back up to the FSM
				return fn_timeout.second(a_ctx);
			}
		}
		return std::nullopt;
	}

	//-------------------------------------------------------------------------------------------

	//std::optional<fsm_state> state_wait_dialog_finished::handle_tick(game::context& a_ctx) {
	//	logger_custom::trace("state_sleep::handle_tick");
	//	if (a_ctx._clock.get_play_second() >= _timeout_p) {
	//		logger_custom::debug("state_sleep::handle_tick");
	//		if (on_timeout.first) {
	//			logger_custom::debug("state_sleep::handle_tick calling timeout callback");
	//			// Forward whatever next state the lambda chooses back up to the FSM
	//			return on_timeout.second(a_ctx);
	//		}
	//	}
	//	return std::nullopt;
	//}

	//std::optional<fsm_state> state_wait_dialog_finished::handle_dialog_finished(game::context& a_ctx, RE::Actor* a_speaker) {
	//	logger_custom::debug("state_sleep::handle_dialog_finished");
	//	if (on_dialog_finished.first) {
	//		logger_custom::debug("state_sleep::handle_dialog_finished calling dialog finished callback");
	//		return on_dialog_finished.second(a_ctx, a_speaker);
	//	}
	//	return std::nullopt;
	//}

	//----------------------------------------------------------------------------------------------

	std::optional<fsm_state> state_dialog_open::handle_tick(game::context& a_ctx) {
		logger_custom::trace("state_dialog_open::handle_tick");
		if (a_ctx._clock.get_play_second() >= _timeout_p) {
			logger_custom::debug("state_dialog_open::handle_tick");
			if (fn_timeout.first) {
				logger_custom::debug("state_dialog_open::handle_tick calling timeout callback");
				// Forward whatever next state the lambda chooses back up to the FSM
				return fn_timeout.second(a_ctx);
			}
		}
		return std::nullopt;
	}

	std::optional<fsm_state> state_dialog_open::handle_dialogmenu_open(game::context& a_ctx, RE::Actor* a_speaker) {
		logger_custom::debug("state_dialog_open::handle_dialogmenu_open");
		if (fn_menu_open.first && a_speaker && a_speaker->GetHandle() == _speaker) {
			logger_custom::debug("state_dialog_open::handle_dialogmenu_open calling dialog open callback");
			return fn_menu_open.second(a_ctx, a_speaker);
		}
		return std::nullopt;
	}

	//-------------------------------------------------------------------------------------------------

	std::optional<fsm_state> state_dialog_close::handle_topicinfo_begin(game::context& a_ctx, RE::Actor* a_speaker, RE::FormID a_topicinfo) {
		logger_custom::debug("state_dialog_close::handle_topicinfo_begin");
		if (fn_topicinfo_begin.first) {
			logger_custom::debug("state_dialog_close::handle_topicinfo_begin calling topic begin callback");
			return fn_topicinfo_begin.second(a_ctx, a_speaker, a_topicinfo);
		}
		return std::nullopt;
	}
	
	std::optional<fsm_state> state_dialog_close::handle_dialogmenu_close(game::context& a_ctx) {
		logger_custom::debug("state_dialog_close::handle_dialogmenu_closed");
		if (fn_menu_close.first) {
			logger_custom::debug("state_dialog_close::handle_dialogmenu_closed calling dialog close callback");
			return fn_menu_close.second(a_ctx);
		}
		return std::nullopt;
	}








	std::optional<fsm_state> state_poll::handle_tick(game::context& a_ctx) {
		logger_custom::trace("state_poll::handle_tick");
		if (fn_poll.first)
			return fn_poll.second(a_ctx);
		return std::nullopt;
	}
	

	std::optional<fsm_state> state_scene_end::handle_tick(game::context& a_ctx) {
		if (a_ctx._clock.get_play_second() >= _timeout_p) {
			logger_custom::debug("state_scene_end::handle_tick");
			if (fn_timeout.first) {
				logger_custom::debug("state_scene_end::handle_tick calling timeout callback");
				// Forward whatever next state the lambda chooses back up to the FSM
				return fn_timeout.second(a_ctx);
			}
		}
		return std::nullopt;
	}

	std::optional<fsm_state> state_scene_end::handle_mod_event(game::context& a_ctx, RE::BSFixedString a_event_name) {
		if (a_event_name == _event_name && fn_modevent.first) {
			logger_custom::debug("state_scene_end::handle_mod_event calling received callback");
			return fn_modevent.second(a_ctx);
		}
		return std::nullopt;
	}

	std::pair<bool, std::optional<fsm_state>> state_scene_end::handle_set_scene_thread_id(game::context& ctx, float a_handle, int32_t a_thread_id) {
		logger_custom::debug("state_scene_end::handle_set_scene_thread_id id={}", a_thread_id);
		if (a_handle == _handle && fn_papyrus.first) {
			logger_custom::debug("state_scene_end::handle_set_scene_thread_id calling callback setter");
			return fn_papyrus.second(ctx, a_handle, a_thread_id);
		}
		return std::make_pair(false, std::nullopt);
	}


}
