#pragma once

#include "fsm_state_fwd.h"

namespace game { struct context; }

namespace fsm {

	struct state_serve_scene_parameters {
		float _timeout_p = 0.f;
		float _handle = 0.f;
		std::vector<RE::Actor*> _actors;
		std::vector<RE::Actor*> _submissives;

		std::function<std::optional<fsm_state>(game::context& ctx)> on_error;
		std::function<std::optional<fsm_state>(game::context& ctx)> on_success;

		state_serve_scene_parameters(float t, float a_handle, std::vector<RE::Actor*> a_actors, std::vector<RE::Actor*> a_submissives,
			std::function<std::optional<fsm_state>(game::context& ctx)> a_error_cb, std::function<std::optional<fsm_state>(game::context& ctx)> a_success_cb)
			: _timeout_p(t), _handle(a_handle), _actors(std::move(a_actors)), _submissives(std::move(a_submissives)), on_error(std::move(a_error_cb)), 
			on_success(std::move(a_success_cb)) {
		}

		std::optional<fsm_state> handle_tick(game::context& a_ctx);
		std::pair<bool, std::optional<fsm_state>> handle_get_scene_parameters(game::context& ctx, RE::TESQuest* a_this, float a_handle);
	};


	struct state_wait_scene_started {
		float _timeout_p = 0.f;
		float _handle = 0.f;

		std::function<std::optional<fsm_state>(game::context& ctx)> on_error;
		std::function<std::optional<fsm_state>(game::context& ctx)> on_success;

		state_wait_scene_started(float t, float a_handle, std::function<std::optional<fsm_state>(game::context& ctx)> a_error_cb,
			std::function<std::optional<fsm_state>(game::context& ctx)> a_success_cb)
			: _timeout_p(t), _handle(a_handle), on_error(std::move(a_error_cb)), on_success(std::move(a_success_cb)) {
		}

		std::optional<fsm_state> handle_tick(game::context& a_ctx);
		std::pair<bool, std::optional<fsm_state>> handle_set_scene_thread_id(game::context& ctx, float a_handle, uint32_t a_thread_id);
		std::pair<bool, std::optional<fsm_state>> handle_set_scene_error(game::context& ctx, float a_handle);
	};


}
