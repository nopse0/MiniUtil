#pragma once

#include "fsm_state_fwd.h"
#include "game_context.h"

namespace fsm {

	class fsm;

	class fsm {
		fsm_state _current_state;

	public:
		fsm();

		fsm_state& get_state() { return _current_state; }
		
		void set_state(fsm_state&& a_loaded_state) {
			_current_state = std::move(a_loaded_state);
		}

		void process_timer_tick(game::context& a_ctx);
		void process_dialog_finished(game::context& a_ctx, RE::Actor* a_speaker);
		void process_mod_event(game::context& a_ctx, RE::BSFixedString a_event_name);

		bool process_get_scene_parameters(game::context& a_ctx, RE::TESQuest* a_this, float a_handle);
		bool process_set_scene_thread_id(game::context& a_ctx, float a_handle, int32_t a_thread_id);
	};



}
