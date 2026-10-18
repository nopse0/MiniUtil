

namespace scheduler {
	class clock {
	private:
		float _play_second = 0.f;    // Time played since the game session started, in seconds.
	public:
		inline void update_play_second(float a_play_time_delta_seconds) { _play_second += a_play_time_delta_seconds; }
		inline float get_play_second() const { return _play_second; }
		static inline float get_game_day()
		{
			auto calendar = RE::Calendar::GetSingleton();
			if (!calendar) return 0.f;
			return calendar->GetDaysPassed();
		}
	};



	// Used for implementing a minimum delay between events
	template <typename Clock>
	class event_delayer {
	private:
		float _event_min_success_delay_game_days = 1.f / 48.f;  // delay after an event has been performed successfully until the next event can trigger (game time)
		float _event_min_success_delay_play_seconds = 120.f;  // delay after an event has been performed successfully until the next event can trigger (play time)
		float _event_min_failure_delay_play_seconds = 20.f;   // some cooldown after a failed attempt to execute an event (e.g. a force greet), to prevent race conditions

		float _next_event_min_game_day = 0.f;        // next point in time, when an event is possible (game time)
		float _next_event_min_play_second = 0.f;     // next point in time, when an event is possible (play time)
	
	public:
		inline void set_delay_success()
		{
			_next_event_min_game_day = this->get_game_day() + _event_min_success_delay_game_days;
			_next_event_min_play_second = this->get_play_second() + _event_min_success_delay_play_seconds;
		}

		inline void set_delay_failure()
		{
			_next_event_min_play_second = this->get_play_second() + _event_min_failure_delay_play_seconds;
		}
	};


	// Used for continuing something after a while ("Sleep" functionality)
	template <typename Clock>
	class sleep_timer {
	private:
		struct task {
			float execute_play_second;
			std::function<void()> fn;
		};
		std::vector<task> _tasks;
	public:
		inline void add_sleep_task(float delay, std::function<void()> fn) {
			_tasks.push_back({ this->get_play_second() + delay, std::move(fn) });
		}
	};


	using time_manager = sleep_timer<event_delayer<clock>>;
	//static GameEventManager test;
}
