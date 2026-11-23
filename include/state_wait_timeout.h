#pragma once

#include "fsm_state_fwd.h"
#include "event_chain_base.h"

namespace fsm {
	class fsm;

	struct state_sleep {
		float _timeout_p = 0.f;
		
		std::pair<uint32_t, std::function<std::optional<fsm_state>(game::context& a_context)>> fn_timeout;

		// Standard constructor
		state_sleep(float t,
			std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context)>> timeout_cb)
			:
			_timeout_p(t),
			fn_timeout(std::move(timeout_cb)) {
		}

		std::optional<fsm_state> handle_tick(game::context& a_ctx);
	};

	struct state_dialog_open {
		float _timeout_p = 0.f;
		RE::ActorHandle _speaker;

		std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context)>> fn_timeout; // = { 0, nullptr };
		std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context, RE::Actor* a_speaker)>> fn_menu_open; // = { 0, nullptr };

		// Standard constructor
		state_dialog_open(float t, const RE::ActorHandle& a_speaker,
			std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context)>> a_timeout_cb,
			std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context, RE::Actor* a_speaker)>> a_menu_open_cb)
			:
			_timeout_p(t),
			_speaker(a_speaker),
			fn_timeout(std::move(a_timeout_cb)),
			fn_menu_open(std::move(a_menu_open_cb)) {
		}

		std::optional<fsm_state> handle_tick(game::context& a_ctx);
		std::optional<fsm_state> handle_dialogmenu_open(game::context& a_ctx, RE::Actor* a_speaker);
	};

	struct state_dialog_close {
		std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context, RE::Actor* a_speaker, RE::FormID a_topicinfo)>> fn_topicinfo_begin;
		std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context)>> fn_menu_close;

		// Standard constructor
		state_dialog_close(
			std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context, RE::Actor* a_speaker, RE::FormID a_topicinfo)>> a_topicinfo_begin_cb,
			std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context)>> a_menu_close_cb)
			:
			fn_topicinfo_begin(std::move(a_topicinfo_begin_cb)),
			fn_menu_close(std::move(a_menu_close_cb)) {
		}

		std::optional<fsm_state> handle_topicinfo_begin(game::context& a_ctx, RE::Actor* a_speaker, RE::FormID a_topicinfo);
		std::optional<fsm_state> handle_dialogmenu_close(game::context& a_ctx);
	};

	struct state_poll {
		std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context)>> fn_poll;

		// Standard constructor
		state_poll(
			std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context)>> a_poll_cb)
			:
			fn_poll(std::move(a_poll_cb)) {
		}

		std::optional<fsm_state> handle_tick(game::context& a_ctx);
	};

	struct state_scene_end {
		float _timeout_p = 0.f;
		float _handle = 0.f;
		RE::BSFixedString _event_name;
		
		std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context)>> fn_timeout;
		std::pair < uint32_t, std::function<std::pair<bool, std::optional<fsm_state>>(game::context& ctx, float a_handle, int32_t a_thread_id)>> fn_papyrus;
		std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context)>> fn_modevent;

		// Standard constructor
		state_scene_end(float a_timeout, float a_handle, RE::BSFixedString a_event_name, 
			std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context)>> a_timeout_cb,
			std::pair < uint32_t, std::function<std::pair<bool, std::optional<fsm_state>>(game::context& ctx, float a_handle, int32_t a_thread_id)>> a_papyrus_cb,
			std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& a_context)>> a_modevent_cb)
			:
			_timeout_p(a_timeout), _handle(a_handle), _event_name(a_event_name),
			fn_timeout(std::move(a_timeout_cb)),
			fn_papyrus(std::move(a_papyrus_cb)),
			fn_modevent(std::move(a_modevent_cb)) {
		}

		std::optional<fsm_state> handle_tick(game::context& a_ctx);
		std::pair<bool, std::optional<fsm_state>> handle_set_scene_thread_id(game::context& ctx, float a_handle, int32_t a_thread_id);
		std::optional<fsm_state> handle_mod_event(game::context& a_ctx, RE::BSFixedString a_event_name);
	};

}
