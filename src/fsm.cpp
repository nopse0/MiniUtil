#include "state_all.h"
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
	//static_assert(IsValidState <state_idle>);
	//static_assert(IsValidState <state_sleep>);
	//static_assert(IsValidState <state_wait_dialog_finished>);
	//static_assert(IsValidState <state_poll>);
	//static_assert(IsValidState <state_serve_scene_parameters>);
	//static_assert(IsValidState <state_wait_scene_started>);
	//static_assert(IsValidState <state_wait_for_mod_event>);
	
	fsm::fsm() : _current_state(::fsm::state_idle{}) {

	}


	void fsm::process_timer_tick(game::context& a_ctx) {
		// 1. Send the event down to the active variant state
		auto next_state = std::visit([&a_ctx](auto& active_state) {
			if constexpr (requires { active_state.handle_tick(a_ctx); }) {
				return active_state.handle_tick(a_ctx);
			}
			else {
				logger_custom::trace("current state didn't handle time tick");
				return std::optional<fsm_state>{};
			}
		}, _current_state);

		// 2. Process transition safely outside the variant execution context
		if (next_state.has_value()) {
			_current_state = std::move(*next_state);
		}
	}
	
	void fsm::process_dialog_finished(game::context& a_ctx, RE::Actor* a_speaker) {
		auto next_state = std::visit([&a_ctx, a_speaker](auto& active_state) {
			if constexpr (requires { active_state.handle_dialog_finished(a_ctx, a_speaker); }) {
				return active_state.handle_dialog_finished(a_ctx, a_speaker);
			}
			else {
				return std::optional<fsm_state>{};
			}
		}, _current_state);

		if (next_state.has_value()) {
			_current_state = std::move(*next_state);
		}

	}

	void fsm::process_mod_event(game::context& a_ctx, RE::BSFixedString a_event_name) {
		auto next_state = std::visit([&a_ctx, a_event_name](auto& active_state) {
			if constexpr (requires { active_state.handle_mod_event(a_ctx, a_event_name); }) {
				return active_state.handle_mod_event(a_ctx, a_event_name);
			}
			else {
				return std::optional<fsm_state>{};
			}
			}, _current_state);

		if (next_state.has_value()) {
			_current_state = std::move(*next_state);
		}

	}

	bool fsm::process_get_scene_parameters(game::context& a_ctx, RE::TESQuest* a_this, float a_handle) {
		auto result = std::visit([&a_ctx, a_this, a_handle](auto& active_state) {
			if constexpr (requires { active_state.handle_get_scene_parameters(a_ctx, a_this, a_handle); }) {
				return active_state.handle_get_scene_parameters(a_ctx, a_this, a_handle);
			}
			else {
				return std::pair(false, std::optional<fsm_state>{});
			}
		}, _current_state);
	
		logger_custom::debug("fsm::process_get_scene_parameters result = {}", result.first);

		if (result.second.has_value()) {
			_current_state = std::move(*result.second);
		}
	
		logger_custom::debug("fsm::process_get_scene_parameters #2 result = {}", result.first);

		return result.first;
	}

	bool fsm::process_set_scene_thread_id(game::context& a_ctx, float a_handle, uint32_t a_thread_id) {
		auto result = std::visit([&a_ctx, a_handle, a_thread_id](auto& active_state) {
			if constexpr (requires { active_state.handle_set_scene_thread_id(a_ctx, a_handle, a_thread_id); }) {
				return active_state.handle_set_scene_thread_id(a_ctx, a_handle, a_thread_id);
			}
			else {
				return std::pair(false, std::optional<fsm_state>{});
			}
			}, _current_state);

		if (result.second.has_value()) {
			_current_state = std::move(*result.second);
		}

		logger_custom::debug("fsm:::process_set_scene_thread_id result = {}", result.first);

		return result.first;
	}

	bool fsm::process_set_scene_error(game::context& a_ctx, float a_handle) {
		auto result = std::visit([&a_ctx, a_handle](auto& active_state) {
			if constexpr (requires { active_state.handle_set_scene_error(a_ctx, a_handle); }) {
				return active_state.handle_set_scene_error(a_ctx, a_handle);
			}
			else {
				return std::pair(false, std::optional<fsm_state>{});
			}
			}, _current_state);

		if (result.second.has_value()) {
			_current_state = std::move(*result.second);
		}

		logger_custom::debug("fsm:::process_set_scene_error result = {}", result.first);

		return result.first;
	}

}

