#pragma once

#include <functional>
#include <vector>

namespace controller {
	enum class state_t {
		Idle,
		ForceGreet,
		Struggle,
		InScene
	};


	class controller : public RE::BSTEventSink<SKSE::ModCallbackEvent>, public singleton<controller> {
	public:
		// Configuration parameters (at the moment constants, so don't have to be persisted across game saves)
		static inline constexpr int timer_ticks_per_hour() { return 36000; } // 36000 ticks per hour (10 ticks per second)
		float _event_min_success_delay_game_days = 1.f / 48.f;  // 30 minutes
		float _event_min_success_delay_play_seconds = 60.f;
		float _event_min_failure_delay_play_seconds = 20.f;   // some cooldown after a failed force greet attempt, to prevent race conditions
		float _force_greet_timeout_play_seconds = 30.f;
		int _struggle_timeout_seconds = 20;
		double _base_event_chance_per_hour = 0.1; // 10% chance per hour

	private:
		// State which has to be persisted across game saves
		state_t _state = state_t::Idle;
		float _play_second = 0.f;    // Time played since the game started, in seconds.
		bool _busy = false;
		float _next_event_min_game_day = 0.f;
		float _next_event_min_play_second = 0.f;
		float _force_greet_start_play_second = 0.f;
		RE::ActorHandle _force_greet_actor_handle;   // = RE::RefHandle(0);   // Careful, we changed the AI package of these actors
		
		// Temporary state	
		RE::ActorHandle _back_hug_actor_ref_handle;    // = RE::RefHandle(0);
		// Scheduler
		struct ScheduledTask {
			float execute_play_second;
			std::function<void()> fn;
		};
		std::vector<ScheduledTask> _scheduledTasks;

		// Concurrency CTD test, is synchronization neccessary or not, when processing mod events
		//double* _concurrencyTestData = new double(1.0);

	public:
		void reset();
		void save(SKSE::SerializationInterface* a_serde);

		void on_timer_tick(float a_play_time_delta_seconds);
		virtual RE::BSEventNotifyControl ProcessEvent(const SKSE::ModCallbackEvent* a_event, RE::BSTEventSource<SKSE::ModCallbackEvent>* a_eventSource);
		void update_play_second(float a_play_time_delta_seconds);
		float get_play_second() { return _play_second; }
		void set_event_success_min_delays();
		void set_event_failure_min_delays();
		void on_dialog_end(RE::Actor* a_speaker);
		RE::ActorHandle start_back_hug(RE::Actor* a_actor);
		void stop_back_hug(RE::Actor* a_actor);

		// Scheduling API
		void schedule_task_after_play_seconds(float a_delay, std::function<void()> a_fn);
		void schedule_task_at_play_second(float a_execute_play_second, std::function<void()> a_fn);
	};
}
