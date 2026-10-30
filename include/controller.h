#pragma once

#include "clock.h"
#include "fsm.h"
#include "events.h"

namespace fsm { class fsm; }

namespace controller {


	class controller : public RE::BSTEventSink<SKSE::ModCallbackEvent>, public singleton<controller> {
	public:
		controller();

		// SKSE Serialization
		void revert();
		bool serialize(SKSE::SerializationInterface* a_serde);
		int32_t deserialize(SKSE::SerializationInterface* a_serde);

		// Mod event callback
		virtual RE::BSEventNotifyControl ProcessEvent(const SKSE::ModCallbackEvent* a_event, RE::BSTEventSource<SKSE::ModCallbackEvent>* a_eventSource);
		
		// Functions called from Papyrus scripts
		void on_dialog_end(RE::Actor* a_speaker);
		bool on_get_scene_parameters(RE::TESQuest* a_this, float a_handle);
		bool on_set_scene_thread_id(float a_handle, int32_t a_thread_id);

		// Skyrim Update loop hook
		uint32_t next_timer_tick(float a_play_time_delta_seconds);
	
	private:
		fsm::fsm _fsm;
		settings::settings _settings;
		timing::clock _clock;
		event_chain::manager _event_manager;
		//minigame::struggle_game _struggle_game;
		sl_scenes::sl_scene_starter _scene_starter;
		game::context _context;
	
	};

}
