#pragma once

#include "fsm_state_fwd.h"
#include "event_chain_base.h"

namespace fsm {
	class fsm;

	struct state_sleep {
		float _timeout_p = 0.f;
		
		std::pair<uint32_t, std::function<std::optional<fsm_state>(game::context& a_context)>> on_timeout;

		// Standard constructor
		state_sleep(float t,
			std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context)>> timeout_cb)
			:
			_timeout_p(t),
			on_timeout(std::move(timeout_cb)) {
		}

		std::optional<fsm_state> handle_tick(game::context& a_ctx);
	};

	struct state_wait_dialog_finished {
		float _timeout_p = 0.f;

		std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context)>> on_timeout; // = { 0, nullptr };
		std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context, RE::Actor* a_speaker)>> on_dialog_finished; // = { 0, nullptr };

		// state_wait_dialog_finished() = default;

		// Standard constructor
		state_wait_dialog_finished(float t, 
			std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context)>> timeout_cb,
			std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context, RE::Actor* a_speaker)>> dialog_cb)
			:
			_timeout_p(t),
			on_timeout(std::move(timeout_cb)),
			on_dialog_finished(std::move(dialog_cb)) {
		}

		std::optional<fsm_state> handle_tick(game::context& a_ctx);
		std::optional<fsm_state> handle_dialog_finished(game::context& a_ctx, RE::Actor* a_speaker);
	};

	struct state_wait_for_mod_event {
		float _timeout_p = 0.f;
		RE::BSFixedString _event_name;
		
		std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context)>> on_timeout;
		std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context)>> on_mod_event_received;

		// Standard constructor
		state_wait_for_mod_event(float t, RE::BSFixedString a_event_name, 
			std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context)>> timeout_cb,
			std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context)>> received_cb)
			:
			_timeout_p(t), _event_name(a_event_name),
			on_timeout(std::move(timeout_cb)),
			on_mod_event_received(std::move(received_cb)) {
		}

		std::optional<fsm_state> handle_tick(game::context& a_ctx);
		std::optional<fsm_state> handle_mod_event(game::context& a_ctx, RE::BSFixedString a_event_name);
	};

	struct state_poll {
		std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context)>> poll;

		// Standard constructor
		state_poll(
			std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context)>> poll_cb)
			: 
			poll(std::move(poll_cb)) {}

		std::optional<fsm_state> handle_tick(game::context& a_ctx);
	};

}
