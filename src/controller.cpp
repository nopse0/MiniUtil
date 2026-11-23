#include "state_all.h"
#include "fsm.h"
#include "controller.h"
#include "struggle_game.h"
#include "scanner.h"
#include "actor_util.h"
#include "form_cache.h"
#include "sl_scenes.h"
#include "random.h"
#include "papyrus_utils.h"
#include "skyrim_util.h"
#include "serialization.h"

namespace controller {
    controller::controller() : _context{_settings, _clock, _event_manager, minigame::struggle_game::get_instance(), _scene_starter, random::random::get_instance()} {
        _event_manager.initialize(_context);
    }


    // SKSE Serialization
    void controller::revert() {
        _context._clock.set_play_second(0.f);
    }
    bool controller::serialize(SKSE::SerializationInterface* a_serde) {
        logger::info("controller::serialize: saving event_manager");
        if (!_event_manager.serialize(a_serde, _context))
            return false;

        logger::info("controller::serialize: saving fsm state");
        bool success = serialization::SaveState(a_serde, _fsm.get_state());
        
        return success;
    }

    int32_t controller::deserialize(SKSE::SerializationInterface* a_serde) {
        int32_t read_bytes = 0;
        int32_t current_read = 0;
        logger_custom::debug("controller::deserialize");

        current_read = _event_manager.deserialize(a_serde);
        if (current_read < 0) return -1;
        read_bytes += current_read;

        auto ev = _event_manager.get_active_event();
        logger_custom::debug("controller::deserialize read_bytes={} ev={}", read_bytes, (void*)ev);

        fsm::fsm_state state;

        if (ev == nullptr) {
            current_read = serialization::LoadFreeState(a_serde, state);
            if (current_read < 0) return -1; else read_bytes += current_read;
        }
        else {
            // 3. Über std::visit die ECHTE Kette im Vektor als Referenz auspacken
            // Das Sternchen (*ev) dereferenziert den Zeiger für visit
            current_read = std::visit([a_serde, &state](auto& concrete_chain) -> int32_t {
                return serialization::LoadEventState(&concrete_chain, a_serde, state);
                }, *ev);
            if (current_read < 0) return -1; else read_bytes += current_read;
        }
        _fsm.set_state(std::move(state));

        logger_custom::debug("controller::deserialize current_read={} read_bytes={}", current_read, read_bytes);
        return read_bytes;
    }

    // Mod event callback
    RE::BSEventNotifyControl controller::ProcessEvent(const SKSE::ModCallbackEvent* a_event, RE::BSTEventSource<SKSE::ModCallbackEvent>* a_eventSource) {
        if (a_event) {
            logger::trace("controller::ProcessEvent: eventName = {}", a_event->eventName.c_str());
            _fsm.process_mod_event(_context, a_event->eventName);
        }
        return RE::BSEventNotifyControl::kContinue;
    }

    // Functions called from Papyrus scripts
    //void controller::on_dialog_end(RE::Actor* a_speaker) {
    //    _fsm.process_dialog_finished(_context, a_speaker);
    //}

    void controller::on_dialogmenu_open(RE::Actor* a_speaker) {
        _fsm.process_dialogmenu_open(_context, a_speaker);
    }
    
    void controller::on_dialogmenu_close() {
        _fsm.process_dialogmenu_close(_context);
    }
    
    void controller::fn_topicinfo_begin(RE::Actor* a_speaker, RE::FormID a_form_id) {
        _fsm.process_topicinfo_begin(_context, a_speaker, a_form_id);
    }

    bool controller::on_get_scene_parameters(RE::TESQuest* a_this, float a_handle) {
        return _fsm.process_get_scene_parameters(_context, a_this, a_handle);
    }

    bool controller::on_set_scene_thread_id(float a_handle, int32_t a_thread_id) {
        return _fsm.process_set_scene_thread_id(_context, a_handle, a_thread_id);
    }

    // Skyrim Update loop hook
    uint32_t controller::next_timer_tick(float a_play_time_delta_seconds) {

        // only advance clock, if playing
        if (!skyrim_util::is_playing()) {
            logger_custom::trace("controller::next_timer_tick aborted, not playing");
            return 6;
        }

        _clock.update_play_second(a_play_time_delta_seconds);
        logger_custom::trace("controller::next_timer_tick current play time = {}", _clock.get_play_second());

        _fsm.process_timer_tick(_context);
        return 6;  // next tick in 6 frames
    }

}
