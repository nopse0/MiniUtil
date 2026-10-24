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
		RE::ActorHandle _npc1_handle;

		bool trigger_phase1(game::context& ctx);
		std::optional<fsm::fsm_state> trigger_phase2(game::context& ctx);
	};

}
