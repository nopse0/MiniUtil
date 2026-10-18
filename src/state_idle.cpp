#include "state_idle.h"
#include "state_force_greet_1npc.h"
#include "fsm.h"
#include "game_context.h"
#include "events.h"

namespace fsm {

	std::optional<fsm_state> state_idle::handle_tick(fsm& a_fsm, game::context& a_ctx) {
		logger_custom::trace("handle_tick");
		return a_ctx._event_chain_manager.trigger(a_fsm, a_ctx);	
	}
	
	
	std::optional<fsm_state> state_idle::handle_dialog_finished(fsm& a_fsm, game::context& a_ctx, RE::Actor* a_speaker) {
		logger_custom::trace("handle_dialog_finished");
		return std::nullopt;
	}



}
