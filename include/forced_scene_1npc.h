#pragma once

#include "event_chain_base.h"
#include "fsm_state_fwd.h"

// cycle here, need forward declaration
namespace game {
	struct context;
}
// cycle here, need forward declaration
namespace fsm {
	class fsm;
}

namespace event_chain {

	struct forced_scene_1npc : delayed {
		RE::ActorHandle _force_greet_actor_handle;
		bool _trigger = false;

		bool pre_trigger(game::context& ctx);
		void update_cooldown(game::context& ctx);
		std::optional<fsm::fsm_state> trigger(fsm::fsm& fsm, game::context& ctx);
	};

}
