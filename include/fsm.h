#pragma once

#include "fsm_state_fwd.h"
#include "game_context.h"

//#include "state_idle.h"
//#include "state_force_greet_1npc.h"

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

	//using fsm_state = std::variant<state_idle, state_force_greet_1npc>;   //, state_sleep, state_force_greet_one, state_force_greet_two>;

	template <typename T>
	concept IsValidState = requires(T s, fsm& a_fsm, game::context& a_ctx, RE::Actor* a_speaker) {
		{ s.handle_tick(a_fsm, a_ctx) }   -> std::same_as<std::optional<fsm_state>>;
		{ s.handle_dialog_finished(a_fsm, a_ctx, a_speaker) } -> std::same_as<std::optional<fsm_state>>;
	};

	//static_assert(IsValidState<state_idle>);
	//static_assert(IsValidState<state_force_greet_1npc>);

	//struct state_sleep;
	//struct state_force_greet_two;


	class fsm {
		fsm_state _current_state;

	public:
		fsm();

		// T is the specific state type you want to change to
		template <typename T, typename... Args>
		void emplace_state(Args&&... args) {
			// Construct the state directly inside the variant buffer
			_current_state.emplace<T>(std::forward<Args>(args)...);
		}

		//void set_current_state(fsm_state&& a_state);

		void process_timer_tick(game::context& a_ctx);
		void process_dialog_finished(game::context& a_ctx, RE::Actor* a_speaker);
	};


	/*struct state_force_greet_one {
		float _timeout_play = 0.f;
		RE::ActorHandle _actor_handle;
		std::function<void(fsm&, RE::Actor* a_speaker)> on_dialog_finished;
		std::function<void(fsm&)> on_timeout;
	};

	struct state_force_greet_two {
		float _timeout_play = 0.f;
		RE::ActorHandle _actor_handle_one;
		RE::ActorHandle _actor_handle_two;
		std::function<void(fsm&, RE::Actor* a_speaker)> on_dialog_finished;
		std::function<void(fsm&)> on_timeout;
	};*/

	//static_assert(IsValidState<state_force_greet_one>);


}
