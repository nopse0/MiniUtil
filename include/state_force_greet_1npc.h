#pragma once

#include "fsm_state_fwd.h"

namespace game {
	struct context;
}

namespace fsm {
	class fsm;

	struct state_force_greet_1npc {
		float _timeout_p = 0.f;
		std::function<std::optional<fsm_state>(fsm&, game::context& a_context)> on_timeout;
		std::function<std::optional<fsm_state>(fsm&, game::context& a_context, RE::Actor* a_speaker)> on_dialog_finished;

		// Standard constructor
		state_force_greet_1npc(float t, std::function<std::optional<fsm_state>(fsm&, game::context& a_context)> timeout_cb,
			std::function<std::optional<fsm_state>(fsm&, game::context& a_context, RE::Actor* a_speaker)> dialog_cb)
			: _timeout_p(t),
			on_timeout(std::move(timeout_cb)),
			on_dialog_finished(std::move(dialog_cb)) {
		}

		std::optional<fsm_state> handle_tick(fsm& a_fsm, game::context& a_ctx);
		std::optional<fsm_state> handle_dialog_finished(fsm& a_fsm, game::context& a_ctx, RE::Actor* a_speaker);
	};


}
