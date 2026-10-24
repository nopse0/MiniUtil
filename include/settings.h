#pragma once

namespace settings {

	struct delay_def {
		float _cooldown_p = 0.f;
		float _cooldown_g = 0.f;
		float _retry_delay_p = 0.f;
	};

	struct settings {
		float _force_greet_timeout_p = 30.f;
		//float _struggle_game_timeout_seconds = 15.f;
		float _papyrus_sattelite_timeout_seconds = 10.f;
		float _scene_duration_max_seconds = 180.f;

		delay_def _forced_scene_1npc_delays = delay_def{60.f, 0.f, 20.f};
	};
}
