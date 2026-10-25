#include "state_all.h"
#include "game_context.h"

namespace fsm {

	std::optional<fsm_state> state_sleep::handle_tick(game::context& a_ctx) {
		logger_custom::trace("state_sleep::handle_tick");
		if (a_ctx._clock.get_play_second() >= _timeout_p) {
			logger_custom::debug("state_sleep::handle_tick");
			if (on_timeout.first) {
				logger_custom::debug("state_sleep::handle_tick calling timeout callback");
				// Forward whatever next state the lambda chooses back up to the FSM
				return on_timeout.second(a_ctx);
			}
		}
		return std::nullopt;
	}

	std::optional<fsm_state> state_wait_dialog_finished::handle_tick(game::context& a_ctx) {
		logger_custom::trace("state_sleep::handle_tick");
		if (a_ctx._clock.get_play_second() >= _timeout_p) {
			logger_custom::debug("state_sleep::handle_tick");
			if (on_timeout.first) {
				logger_custom::debug("state_sleep::handle_tick calling timeout callback");
				// Forward whatever next state the lambda chooses back up to the FSM
				return on_timeout.second(a_ctx);
			}
		}
		return std::nullopt;
	}

	std::optional<fsm_state> state_wait_dialog_finished::handle_dialog_finished(game::context& a_ctx, RE::Actor* a_speaker) {
		logger_custom::debug("state_sleep::handle_dialog_finished");
		if (on_dialog_finished.first) {
			logger_custom::debug("state_sleep::handle_dialog_finished calling dialog finished callback");
			return on_dialog_finished.second(a_ctx, a_speaker);
		}
		return std::nullopt;
	}

	std::optional<fsm_state> state_poll::handle_tick(game::context& a_ctx) {
		logger_custom::trace("state_poll::handle_tick");
		if (poll.first)
			return poll.second(a_ctx);
		return std::nullopt;
	}
	

	std::optional<fsm_state> state_wait_for_mod_event::handle_tick(game::context& a_ctx) {
		if (a_ctx._clock.get_play_second() >= _timeout_p) {
			logger_custom::debug("state_wait_for_mod_event::handle_tick");
			if (on_timeout.first) {
				logger_custom::debug("state_wait_for_mod_event::handle_tick calling timeout callback");
				// Forward whatever next state the lambda chooses back up to the FSM
				return on_timeout.second(a_ctx);
			}
		}
		return std::nullopt;
	}

	std::optional<fsm_state> state_wait_for_mod_event::handle_mod_event(game::context& a_ctx, RE::BSFixedString a_event_name) {
		if (a_event_name == _event_name && on_mod_event_received.first) {
			logger_custom::debug("state_wait_for_mod_event::handle_mod_event calling received callback");
			return on_mod_event_received.second(a_ctx);
		}
		return std::nullopt;
	}
}
