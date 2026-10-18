#include "state_force_greet_1npc.h"
#include "state_idle.h"
#include "game_context.h"

namespace fsm {

	std::optional<fsm_state> state_force_greet_1npc::handle_tick(fsm& a_fsm, game::context& a_ctx) {
		logger_custom::trace("handle_tick");
		if (a_ctx._clock.get_play_second() >= _timeout_p) {
			if (on_timeout) {
				// Forward whatever next state the lambda chooses back up to the FSM
				return on_timeout(a_fsm, a_ctx);
			}
		}
		return std::nullopt;
	}


	std::optional<fsm_state> state_force_greet_1npc::handle_dialog_finished(fsm& a_fsm, game::context& a_ctx, RE::Actor* a_speaker) {
		logger_custom::debug("handle_tick");
		return on_dialog_finished(a_fsm, a_ctx, a_speaker);
	}

}

