#pragma once

#include "game_context.h"

namespace event_chain {

	struct delayed {
		float _next_play_second = 0.f;
		float _next_game_day = 0.f;
		float _delay_play_seconds = 0.f;
		float _delay_game_days = 0.f;
		float _delay_retry_play_seconds = 0.f;
		inline float get_next_p() const { return _next_play_second; }
		inline float get_next_g() const { return _next_game_day; }
		inline void update_cooldown(game::context& ctx, bool triggered) {
			logger_custom::debug("forced_scene_1npc update cooldown");
			if (!triggered) {
				_next_play_second = ctx._clock.get_play_second() + _delay_retry_play_seconds;
			}
			else {
				_next_play_second = ctx._clock.get_play_second() + _delay_play_seconds;
				_next_game_day = ctx._clock.get_game_day() + _delay_game_days;
			}
		}
	};

}
