#pragma once

#include "fsm_state_fwd.h"

namespace game { struct context; }

namespace fsm {

	struct state_scene_start {
		float _timeout_p = 0.f;
		float _handle = 0.f;

		std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& ctx)>> fn_timeout;
		std::pair < uint32_t, std::function<std::pair<bool, std::optional<fsm_state>>(game::context& ctx, RE::TESQuest* a_this, float a_handle)>> fn_papyrus;

		state_scene_start(float t, float a_handle,
			std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& ctx)>> a_timeout_cb,
			std::pair < uint32_t, std::function<std::pair<bool, std::optional<fsm_state>>(game::context& ctx, RE::TESQuest* a_this, float a_handle)>> a_papyrus_cb)
			: 
			_timeout_p(t), _handle(a_handle),
			fn_timeout(std::move(a_timeout_cb)),
			fn_papyrus(std::move(a_papyrus_cb)) {
			logger_custom::debug("state_scene_start timeout set to {}", _timeout_p);
		}

		std::optional<fsm_state> handle_tick(game::context& a_ctx);
		std::pair<bool, std::optional<fsm_state>> handle_get_scene_parameters(game::context& ctx, RE::TESQuest* a_this, float a_handle);
	};

	// TODO remove
	//struct state_wait_scene_set_thread_id {
	//	float _timeout_p = 0.f;
	//	float _handle = 0.f;

	//	std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& ctx)>> fn_timeout;
	//	std::pair < uint32_t, std::function<std::pair<bool, std::optional<fsm_state>>(game::context& ctx, float a_handle, int32_t a_thread_id)>> on_set;

	//	state_wait_scene_set_thread_id(float t, float a_handle,
	//		std::pair < uint32_t, std::function<std::optional<fsm_state>(game::context& ctx)>> a_timeout_cb,
	//		std::pair < uint32_t, std::function<std::pair<bool, std::optional<fsm_state>>(game::context& ctx, float a_handle, int32_t a_thread_id)>> a_setter_cb)
	//		:
	//		_timeout_p(t), _handle(a_handle), fn_timeout(std::move(a_timeout_cb)), on_set(std::move(a_setter_cb)) {
	//	}

	//	std::optional<fsm_state> handle_tick(game::context& a_ctx);
	//	std::pair<bool, std::optional<fsm_state>> handle_set_scene_thread_id(game::context& ctx, float a_handle, int32_t a_thread_id);
	//};

}
