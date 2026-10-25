#include "state_all.h"
#include "fsm.h"
#include "game_context.h"
#include "events.h"

namespace fsm {
	std::optional<fsm_state> pre_save(game::context& a_ctx) {
		return std::nullopt;
	}

	std::optional<fsm_state> state_idle::handle_tick(game::context& a_ctx) {
		logger_custom::trace("state_idle::handle_tick handle timer tick");		
		return a_ctx._event_chain_manager.trigger(a_ctx);	
	}
	
	std::optional<fsm_state> state_idle::handle_dialog_finished(game::context& a_ctx, RE::Actor* a_speaker) {
		logger_custom::debug("state_idle::handle_dialog_finished handle dialog finished");
		return std::nullopt;
	}



}
