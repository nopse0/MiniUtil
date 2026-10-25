#pragma once

#include "fsm_state_fwd.h"
#include "game_context.h"

namespace fsm {

#if 0
	struct context_t {
		time_manager::clock& clock;
		//events::selector& event_selector;
	};


	struct event_timer_tick {
		context_t& ctx;
	};


	struct state_idle_cooldown {
		float _next_event_min_game_day = 0.f;        // next point in time, when an event is possible (game time)
		float _next_event_min_play_second = 0.f;     // next point in time, when an event is possible (play time)
	};

	struct state_idle_trigger {

	};

	struct state_force_greet_one {
		//events::selector::event event;
	};



	using fsm_state = std::variant<state_idle_cooldown, state_idle_trigger, state_force_greet_one>;

	class fsm_t {
	private:
		fsm_state _current_state = state_idle_cooldown{};

	public:
		void handle_event(auto&& e) {

			fsm_state next_state = std::visit([&](auto& s) -> fsm_state {

				if constexpr (std::is_same_v<std::decay_t<decltype(e)>, event_timer_tick>) {
					if constexpr (std::is_same_v<std::decay_t<decltype(s)>, state_idle_cooldown>) {
						if (e.ctx.clock.get_play_second() < s._next_event_min_play_second || e.ctx.clock.get_game_day() < s._next_event_min_game_day) {
							return s;
						}
						return state_idle_trigger{};
					}
					else if constexpr (std::is_same_v<std::decay_t<decltype(s)>, state_idle_trigger>) {
						switch(e.ctx.event_selector.select()) {
						case events::selector::event::None:
							return state_idle_cooldown{};
						case events::selector::event::ForcedOne:
							return state_force_greet_one{ events::selector::event::ForcedOne };
						case events::selector::event::ForcedTwo:
							return state_idle_cooldown{};
						}
					}
					else if constexpr (std::is_same_v<std::decay_t<decltype(s)>, state_force_greet_one>) {
						return s;
					}

					return s;
				}
				else
					return s;
				}, _current_state);
		}

	};

#endif
	class fsm;

	//template <typename T>
	//concept IsValidState = requires(T s, game::context& a_ctx) {
	//	{ s.pre_save(a_ctx) } -> std::same_as<std::optional<fsm_state>>; // we need this for serialization, return to a state which can be saved
	//};

	class fsm {
		fsm_state _current_state;

	public:
		fsm();

		fsm_state& get_state() { return _current_state; }
		
		void set_state(fsm_state&& a_loaded_state) {
			_current_state = std::move(a_loaded_state);
		}

		// T is the specific state type you want to change to
		//template <typename T, typename... Args>
		//void emplace_state(Args&&... args) {
		//	// Construct the state directly inside the variant buffer
		//	_current_state.emplace<T>(std::forward<Args>(args)...);
		//}

		void process_timer_tick(game::context& a_ctx);
		void process_dialog_finished(game::context& a_ctx, RE::Actor* a_speaker);
		void process_mod_event(game::context& a_ctx, RE::BSFixedString a_event_name);

		bool process_get_scene_parameters(game::context& a_ctx, RE::TESQuest* a_this, float a_handle);
		bool process_set_scene_thread_id(game::context& a_ctx, float a_handle, uint32_t a_thread_id);
		bool process_set_scene_error(game::context& a_ctx, float a_handle);
	};



}
