#pragma once

#include "fsm_state_fwd.h"
#include "forced_scene_1npc.h"
//#include "fsm.h"

//#include "scanner.h"
//#include "random.h"
//#include "time_manager.h"

namespace fsm { class fsm; }

//namespace events {
//
//	struct settings {
//		double base_chance_global = 1.0;
//		double base_chance_mono_greet = 0.0;
//		double base_chance_dual_greet = 1.0;
//		double base_chance_greet_outcome_negative = 1.0;
//	};
//
//	class manager : public singleton<manager> {
//	private:
//		settings _current_settings;
//
//		// Loading old save, in this time the GUI shouldn't interfer with the settings
//		std::atomic<bool> _is_loading{ false };
//
//		// For GUI setting current settings
//		settings _ui_set_settings;
//		std::atomic<bool> _ui_set_settings_updated;    // set by gui thread / read by game thread
//
//		// For GUI reading current settings
//		settings _ui_get_settings;
//		std::atomic<bool> _ui_get_settings_updated;     // set by game thread / read by gui thread
//
//	public:
//		// Called by game thread
//		void game_update_settings();
//		void set_loading_state(bool loading);
//		void deserialize(const settings& loaded_settings);
//		// With this, the GUI checks if it should be disabled (when settings are loaded from save game)
//		bool ui_is_loading() const;
//		// Called by GUI thread to set the current settings
//		void ui_set_settings(const settings& a_settings);
//		// Called by GUI thread to read the current settings. Returns if the settings had been changed since the last call
//		bool ui_get_settings(settings& a_settings);
//
//	};
//}

	//template <typename T>
	//concept IsValidEvent = requires(T ev) {
	//	{ ev.check_trigger() }   -> std::same_as<bool>;
	//	{ ev.next_check_play() } -> std::same_as<float>;
	//	{ ev.next_check_game() } -> std::same_as<float>;
	//	//{ ev.update_cooldown(std::declval<TimePoint>()) } -> std::same_as<void>;
	//};

	//struct event_delay {
	//	float _next_play_second;
	//	float _next_game_day;
	//	float _delay_play_seconds;
	//	float _delay_game_days;
	//};

	//struct forced_one: event_delay {
	//	bool check_trigger() {
	//		return true;
	//	}

	//	float next_check_play() {
	//		return _next_play_second;
	//	}

	//	float next_check_game() {
	//		return _next_game_day;
	//	}
	//
	//};

	//struct forced_two {
	//};

	//using event_variant = std::variant<forced_one, forced_two>;

	//static_assert(IsValidEvent<forced_one>);

	//class selector {
	//public:
	//	enum class event_id_t {
	//		None,
	//		ForcedOne,
	//		ForcedTwo
	//	};

	//	struct event_delay_t {
	//		float _check_interval;
		//	float _cooldown_play;
		//	float _cooldown_game;
		//};

		//struct event_def_t {
		//	event_id_t _id;
		//	event_delay_t _delay;
		//};


		//struct event_delay_play {
		//	float _next_play_second;
		//	float _next_game_day;
		//	float _delay_play_seconds;
		//	float _delay_game_days;
		//	event_id_t _id;
		//};


		//std::vector<event_delay_play> _events;
		//std::vector<event_delay_play>::const_iterator _events_min_it = _events.end();

		//std::vector<event_delay_play> _events_game_delay;
		//std::vector<event_def_t> _event_defs;

		//inline bool event_less(const event_delay_play& lhs, const event_delay_play& rhs) {
		//	if (lhs._next_play_second < rhs._next_play_second)
		//		return true;
		//	else if (lhs._next_play_second > rhs._next_play_second)
		//		return false;
		//	else
		//		return lhs._next_game_day < rhs._next_game_day;
		//}

	//	inline void initialize(const std::vector<event_def_t>& a_event_defs, const time_manager::clock& a_clock) {
	//		_event_defs = a_event_defs;

	//		auto nowp = a_clock.get_play_second();
	//		auto nowg = a_clock.get_game_day();
	//		for (auto& ed : _event_defs) {
	//			event_delay_play delay{ nowp + ed._delay._cooldown_play, nowg + ed._delay._cooldown_game, ed._delay._cooldown_play, ed._delay._cooldown_game, ed._id };
	//			_events.push_back(std::move(delay));
	//		}

	//	}


	//	inline event_id_t select(const time_manager::clock& a_clock) {
	//		if (_events_min_it == _events.end())
	//			_events_min_it = std::min_element(_events.begin(), _events.end(), event_less);
	//		if (_events_min_it == _events.end())
	//			return event_id_t::None;



	//		auto roll = random::random::get_instance().gen_float(0.f, 1.f);


	//		// Are there npcs around ?
	//		std::vector< std::pair<float, RE::Actor*> > males;
	//		std::vector< std::pair<float, RE::Actor*> > females;
	//		auto result = scanner::scan_actors(2000.f, males, females);
	//		if (result && males.size()) {
	//			int n = scanner::nth_nearest(3, males);


	//			return event::ForcedOne;
	//		}
	//	};
	//}

namespace event_chain {

	template <typename T>
	concept IsValidEventChain = requires(T ec, game::context& ctx, SKSE::SerializationInterface * a_serde) {
		{ ec.trigger_phase1(ctx) } -> std::same_as<bool>;
		{ ec.get_next_p() } -> std::same_as<float>;
		{ ec.get_next_g() } -> std::same_as<float>;
		{ ec.set_retry_cooldown(ctx) } -> std::same_as<void>;
		{ ec.serialize(a_serde) } -> std::same_as<bool>;
	};

	using event_variant = std::variant<forced_scene_1npc::forced_scene_1npc>;

	static_assert(IsValidEventChain<forced_scene_1npc::forced_scene_1npc>);

	class manager
	{
		std::vector<event_variant> _events;
	
	public:
		void initialize(game::context& ctx);
		std::optional<fsm::fsm_state> trigger(game::context& ctx);

		bool serialize(SKSE::SerializationInterface* a_serde);
		int32_t deserialize(SKSE::SerializationInterface* a_serde);

		event_variant* get_active_event();

	private:
		float get_next_trigger_time();
	};

}
