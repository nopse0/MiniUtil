#pragma once

namespace timing {
	class clock {
	private:
		float _play_second = 0.f;    // Time played since the game session started, in seconds.
	public:
		inline void set_play_second(float a_play_time_second) { _play_second += a_play_time_second; }
		inline void update_play_second(float a_play_time_delta_seconds) { _play_second += a_play_time_delta_seconds; }
		inline float get_play_second() const { return _play_second; }
		static inline float get_game_day()
		{
			auto calendar = RE::Calendar::GetSingleton();
			if (!calendar) return 0.f;
			return calendar->GetDaysPassed();
		}
	};

}
