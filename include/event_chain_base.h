#pragma once

namespace event_chain {

	struct delay_def {
		float _cooldown_p = 0.f;
		float _cooldown_g = 0.f;
		float _retry_delay_p = 0.f;
	};

	struct delayed {
		float _next_play_second = 0.f;
		float _next_game_day = 0.f;
		float _delay_play_seconds = 0.f;
		float _delay_game_days = 0.f;
		float _delay_retry_play_seconds = 0.f;
		inline float get_next_p() const { return _next_play_second; }
		inline float get_next_g() const { return _next_game_day; }
	};

}
