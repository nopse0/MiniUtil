#pragma once

#include "fsm_state_fwd.h"

namespace game { struct context; }

namespace fsm {
	class fsm;

	struct state_idle {
		std::optional<fsm_state> handle_tick(fsm& a_fsm, game::context& a_ctx);
		std::optional<fsm_state> handle_dialog_finished(fsm& a_fsm, game::context& a_ctx, RE::Actor* a_speaker);
	};


}
