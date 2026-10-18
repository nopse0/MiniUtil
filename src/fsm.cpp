#include "state_idle.h"
#include "state_force_greet_1npc.h"
#include "fsm.h"

#if 0
struct game_context {


};

struct state_idle_cooldown {

};

struct state_idle_trigger {

};


struct event_timer_tick {

};


using fsm_state = std::variant<state_idle_cooldown, state_idle_trigger>;

class fsm_t {
private:
	fsm_state _current_state = state_idle_cooldown{};

public:
	void handle_event(auto&& event) {

		fsm_state next_state = std::visit([&](auto& s) -> fsm_state {

			if constexpr (std::is_same_v<std::decay_t<decltype(event)>, event_timer_tick>) {
				if constexpr (std::is_same_v<std::decay_t<decltype(s)>, state_idle_cooldown>) {
					return state_idle_trigger{};
				}
				else
					return s;
			}
			else
				return s;
		}, _current_state);
	}

};


void test()
{
	fsm_t fsm;

	fsm.handle_event(event_timer_tick{});
}
#endif

namespace fsm {
	static_assert(IsValidState<state_idle>);
	static_assert(IsValidState<state_force_greet_1npc>);


	fsm::fsm() {

	}

	void fsm::process_timer_tick(game::context& a_ctx) {
		// 1. Send the event down to the active variant state
		auto next_state = std::visit([this, &a_ctx](auto& active_state) {
			return active_state.handle_tick(*this, a_ctx);
			}, _current_state);

		// 2. Process transition safely outside the variant execution context
		if (next_state.has_value()) {
			_current_state = std::move(*next_state);
			// Optional: trigger OnEnter() for the new state here
		}
	}
	
	void fsm::process_dialog_finished(game::context& a_ctx, RE::Actor* a_speaker) {
		auto next_state = std::visit([this, &a_ctx, a_speaker](auto& active_state) {
			return active_state.handle_dialog_finished(*this, a_ctx, a_speaker);
			}, _current_state);

		if (next_state.has_value()) {
			_current_state = std::move(*next_state);
		}

	}
}

