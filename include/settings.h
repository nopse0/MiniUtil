#pragma once

#include "event_chain_base.h"

namespace settings {

	struct settings {
		float _force_greet_timeout_p = 30.f;

		event_chain::delay_def _forced_scene_1npc_delays = event_chain::delay_def{120.f, 0.f, 20.f};
	};
}
