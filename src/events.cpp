//#include "time_manager.h"
#include "game_context.h"
//#include "fsm.h"
#include "events.h"
#include "state_idle.h"
#include "state_wait_timeout.h"
#include "state_serve_scene_parameters.h"

#if 0
namespace events {

	// Called by game thread
	void manager::game_update_settings()
	{
		if (_is_loading.load(std::memory_order_relaxed)) {
			return;
		}

		if (_ui_set_settings_updated.load(std::memory_order_acquire))
		{
			_current_settings = _ui_set_settings;
			_ui_set_settings_updated.store(false, std::memory_order_release);

			_ui_get_settings = _current_settings;
			_ui_get_settings_updated.store(true, std::memory_order_release);
		}
	}

	void manager::set_loading_state(bool loading)
	{
		_is_loading.store(loading, std::memory_order_release);
	}

	void manager::deserialize(const settings& loaded_settings)
	{

		// Disable GUI
		// _is_loading.store(loading, std::memory_order_release);

		// Instead of passing in the new settings, we could also do the SKSE deserialization right here. But set_loading_state(true) must still be called beforehand!
		// ...

		_current_settings = loaded_settings;

		// 2. Ausstehende GUI-Schreibbefehle verwerfen, die VOR dem Laden gesendet wurden (also the GUI must not change these while _is_loading==true) !!!!
		_ui_set_settings_updated.store(false, std::memory_order_release);

		_ui_get_settings = _current_settings;
		_ui_get_settings_updated.store(true, std::memory_order_release);
	}

	// With this, the GUI checks if it should be disabled (when settings are loaded from save game)
	bool manager::ui_is_loading() const
	{
		return _is_loading.load(std::memory_order_acquire);
	}


	// Called by GUI thread to set the current settings
	void manager::ui_set_settings(const settings& a_settings)
	{
		if (_is_loading.load(std::memory_order_relaxed)) {
			return;
		}

		_ui_set_settings = a_settings;
		_ui_set_settings_updated.store(true, std::memory_order_release);
	}

	// Called by GUI thread to read the current settings. Returns if the settings had been changed since the last call
	bool manager::ui_get_settings(settings& a_settings)
	{
		if (_ui_get_settings_updated.load(std::memory_order_acquire))
		{
			a_settings = _ui_get_settings;;
			_ui_get_settings_updated.store(false, std::memory_order_release);
			return true;
		}
		return false; // no change
	}

}

namespace experiment {

	template <typename T>
	concept IsValidEvent = requires(T ev) {
		{ ev.check_trigger() }   -> std::same_as<bool>;
		{ ev.next_check_play() } -> std::same_as<float>;
		{ ev.next_check_game() } -> std::same_as<float>;
		//{ ev.update_cooldown(std::declval<TimePoint>()) } -> std::same_as<void>;
	};

	struct event_delay_definition {
		float _check_interval;
		float _cooldown_play;
		float _cooldown_game;
	};

	struct event_delay {
		float _next_play_second;
		float _next_game_day;
		float _delay_play_seconds;
		float _delay_game_days;
	};

	struct forced_one : event_delay {
		bool check_trigger() {
			return true;
		}

		float next_check_play() const {
			return _next_play_second;
		}

		float next_check_game() const {
			return _next_game_day;
		}

	};

	struct forced_two {
	};

	using event_variant = std::variant<forced_one>;

	static_assert(IsValidEvent<forced_one>);

	class event_manager
	{
		std::vector<event_variant> _events;
		//std::vector<event_variant>::iterator _events_min_it = _events.end();

		void initialize(
			const time_manager::clock& a_clock,
			const event_delay_definition& a_forced_one_def
		) {
			auto nowp = a_clock.get_play_second();
			auto nowg = a_clock.get_game_day();
			
			auto& ed = a_forced_one_def;
			forced_one ev { nowp + ed._cooldown_play, nowg + ed._cooldown_game, ed._cooldown_play, ed._cooldown_game };
			
			_events.push_back(std::move(ev));
		}

		static inline bool event_less(const event_variant& lhs, const event_variant& rhs) {
			auto lhs_play = std::visit([](const auto& ev) { return ev.next_check_play(); }, lhs);
			auto rhs_play = std::visit([](const auto& ev) { return ev.next_check_play(); }, rhs);
			if (lhs_play == rhs_play) {
				auto lhs_game = std::visit([](const auto& ev) { return ev.next_check_game(); }, lhs);
				auto rhs_game = std::visit([](const auto& ev) { return ev.next_check_game(); }, rhs);
				return lhs_game < rhs_game;
			}
			else lhs_play < rhs_play;
		}

		void get_next_event() {
			//if (_events_min_it == _events.end())
			auto it = std::min_element(_events.begin(), _events.end(), event_less);
			//if (_events_min_it == _events.end())
				//return;
			bool trigger = std::visit([](auto& ev) -> bool { return ev.check_trigger(); }, *it);

		}

	};


}
#endif

namespace event_chain {

	static inline bool event_less(const event_variant& lhs, const event_variant& rhs) {
		auto lhs_play = std::visit([](const auto& ev) { return ev.get_next_p(); }, lhs);
		auto rhs_play = std::visit([](const auto& ev) { return ev.get_next_p(); }, rhs);
		if (lhs_play == rhs_play) {
			auto lhs_game = std::visit([](const auto& ev) { return ev.get_next_g(); }, lhs);
			auto rhs_game = std::visit([](const auto& ev) { return ev.get_next_g(); }, rhs);
			return lhs_game < rhs_game;
		}
		else return lhs_play < rhs_play;
	}

	void manager::initialize(game::context& ctx) {
		auto nowp = ctx._clock.get_play_second();
		auto nowg = ctx._clock.get_game_day();

		auto& ed = ctx._settings._forced_scene_1npc_delays;
		auto fs = forced_scene_1npc { nowp + ed._cooldown_p, nowg + ed._cooldown_g, ed._cooldown_p, ed._cooldown_g, ed._retry_delay_p };

		_events.push_back(std::move(fs));
	}

	float manager::get_next_trigger_time() {
		auto it = std::min_element(_events.begin(), _events.end(), event_less);
		if (it != _events.end()) {
			return std::visit([](const auto& ev) { return ev.get_next_p(); }, *it);
		}
		else
			return NAN;
	}

	std::optional<fsm::fsm_state> manager::trigger(game::context& ctx) {
		auto it = std::min_element(_events.begin(), _events.end(), event_less);
		if (it == _events.end())
			return std::nullopt;
		auto next_p = std::visit([](const auto& ev) { return ev.get_next_p(); }, *it);
		if (next_p > ctx._clock.get_play_second())
			return std::nullopt;
		bool triggered = std::visit([&ctx](auto& ev) -> bool { return ev.trigger_phase1(ctx); }, *it);
		if (!triggered) {
			std::visit([&ctx](auto& ev) -> void { ev.update_cooldown(ctx, false); }, *it);
			logger_custom::debug("manager::trigger !triggered, next trigger check at play time second {}", get_next_trigger_time()); // clumsy to log this, but useful for debugging
			return std::nullopt;
		}
		else {
			auto result = std::visit([&ctx](auto& ev) -> std::optional<fsm::fsm_state> { return ev.trigger_phase2(ctx); }, *it);
			logger_custom::debug("manager::trigger triggered at play time second {}, scheduled time was {}", ctx._clock.get_play_second(), get_next_trigger_time());
			return result;
		}
	}


}

