#include "state_idle.h"
#include "state_wait_timeout.h"
#include "state_serve_scene_parameters.h"
#include "fsm.h"
#include "game_context.h"
#include "events.h"

namespace fsm {

	std::optional<fsm_state> state_idle::handle_tick(game::context& a_ctx) {
		logger_custom::trace("state_idle::handle_tick handle timer tick");
		//auto spdl = spdlog::get("fsm");
		//if (spdl) {
		//	logger::info("logger(fsm) level = {}", (uint32_t)spdl->level());
		//}
		
		return a_ctx._event_chain_manager.trigger(a_ctx);	
	}
	
	
	std::optional<fsm_state> state_idle::handle_dialog_finished(game::context& a_ctx, RE::Actor* a_speaker) {
		logger_custom::debug("state_idle::handle_dialog_finished handle dialog finished");
		return std::nullopt;
	}



}
