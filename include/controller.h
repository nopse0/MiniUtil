#pragma once

#include <functional>
#include <vector>

namespace controller {
	enum class state_t : uint8_t {
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
		float _scene_timeout_play_seconds = 180.f;

	private:
		// State which has to be persisted across game saves
		state_t _state = state_t::Idle;
		float _play_second = 0.f;    // Time played since the game started, in seconds.
		bool _busy = false;
		float _next_event_min_game_day = 0.f;
		float _next_event_min_play_second = 0.f;
		float _force_greet_start_play_second = 0.f;
		RE::ActorHandle _force_greet_actor_handle;   // Careful, we changed the AI package of these actor

		// Temporary state	
		RE::ActorHandle _back_hug_actor_handle;
		// Scheduler
		struct ScheduledTask {
			float execute_play_second;
			std::function<void()> fn;
		};
		std::vector<ScheduledTask> _scheduledTasks;
		uint8_t _sl_hook_id = 0;
		RE::BSFixedString _sl_hook_name;
		float _scene_start_play_second = 0.f;

	public:
		void revert();
		void serialize(SKSE::SerializationInterface* a_serde);
		uint32_t deserialize(SKSE::SerializationInterface* a_serde);

		void on_timer_tick(float a_play_time_delta_seconds);
		virtual RE::BSEventNotifyControl ProcessEvent(const SKSE::ModCallbackEvent* a_event, RE::BSTEventSource<SKSE::ModCallbackEvent>* a_eventSource);
		void update_play_second(float a_play_time_delta_seconds);
		float get_play_second() { return _play_second; }
		void set_event_success_min_delays();
		void set_event_failure_min_delays();
		void on_dialog_end(RE::Actor* a_speaker);
		RE::ActorHandle start_back_hug(RE::Actor* a_actor);
		void stop_back_hug(RE::Actor* a_actor);
		std::string next_sl_hook();
		void goto_idle_state();

		// Scheduling API
		void schedule_task_after_play_seconds(float a_delay, std::function<void()> a_fn);
		void schedule_task_at_play_second(float a_execute_play_second, std::function<void()> a_fn);
	};
}
