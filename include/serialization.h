#pragma once

#include "fsm_loader.h"

namespace serialization {
    namespace save_blocker {
        inline std::atomic<bool> _block_saving{ false };

        inline void block() noexcept { _block_saving.store(true, std::memory_order_release); }
        inline void unblock() noexcept { _block_saving.store(false, std::memory_order_release); }
        inline bool is_blocked() noexcept { return _block_saving.load(std::memory_order_acquire); }
    
    
        namespace detail {
            void install_hook();
 

        }
    
    
    };



    bool SaveString(SKSE::SerializationInterface* a_intfc, const RE::BSFixedString& a_string);
    bool SaveNiPoint3(SKSE::SerializationInterface* a_intfc, const RE::NiPoint3& a_point);
    bool SaveActorHandle(SKSE::SerializationInterface* a_intfc, const RE::ActorHandle& a_handle);
    int32_t LoadActorHandle(SKSE::SerializationInterface* a_serde, RE::ActorHandle& a_handle);
    int32_t LoadString(SKSE::SerializationInterface* a_intfc, RE::BSFixedString& a_outString);
    int32_t LoadNiPoint3(SKSE::SerializationInterface* a_intfc, RE::NiPoint3& a_point);

    // This enum, and the indices used by the fsm_state std::variant (see fsm_state_fwd.h) must be kept synchronous !!!

    //using fsm_state = std::variant<
    //    std::monostate,
    //    state_idle,
    //    state_sleep,
    //    state_wait_dialog_finished,
    //    state_poll,
    //    state_serve_scene_parameters,
    //    state_wait_scene_started,
    //    state_wait_for_mod_event
    //>;

    enum class StateId {
        None = 0,
        Idle,
        Sleep,
        DialogOpen,
        Poll,
        SceneStart,
        SceneEnd
    };

    int32_t LoadFreeState(SKSE::SerializationInterface* a_serde, fsm::fsm_state& out_state);

    template<typename ChainType>
    int32_t LoadEventState(ChainType* active_chain, SKSE::SerializationInterface* a_serde, fsm::fsm_state& out_state /* Oder aus Savegame lesen */) {

        // 1. Lies den State-Index/Typ aus dem Savegame (Simuliert)
        uint32_t loaded_state_type = 0;
        int32_t read_bytes = 0;
        read_bytes = a_serde->ReadRecordData(&loaded_state_type, sizeof(loaded_state_type));
        if (read_bytes != sizeof(loaded_state_type)) {
            logger::critical("LoadEventState error reading state type");
            out_state = std::monostate{};
            return -1;
        }

        // 2. Der Switch bestimmt, WELCHE Daten jetzt aus dem Savegame gelesen werden müssen
        switch (loaded_state_type) {

        case static_cast<uint32_t>(StateId::None):
            out_state = std::monostate{};
            return read_bytes;

        case static_cast<uint32_t>(StateId::Idle):
            out_state = fsm::state_idle{};
            return read_bytes;

        case static_cast<uint32_t>(StateId::DialogOpen): {
            uint32_t timeout_cb_id = 0;
            uint32_t dialog_opened_cb_id = 0;
            float timeout_val = 0.f;

            uint32_t current_read = 0;
            bool success = true;

            current_read = a_serde->ReadRecordData(&timeout_val, sizeof(timeout_val));
            if (current_read != sizeof(timeout_val)) success = false; else read_bytes += current_read;
            RE::ActorHandle speaker;
            current_read = LoadActorHandle(a_serde, speaker);
            if (current_read < 0) success = false; else read_bytes += current_read;
            
            current_read = a_serde->ReadRecordData(&timeout_cb_id, sizeof(timeout_cb_id));
            if (current_read != sizeof(timeout_cb_id)) success = false; else read_bytes += current_read;
            current_read = a_serde->ReadRecordData(&dialog_opened_cb_id, sizeof(dialog_opened_cb_id));
            if (current_read != sizeof(dialog_opened_cb_id)) success = false; else read_bytes += current_read;

            if (!success) {
                logger::critical("LoadEventState state_dialog_open error");
                out_state = std::monostate{};
                return -1;
            }

            out_state = fsm::loader::create_state_dialog_open(
                active_chain,
                timeout_val,
                speaker,
                timeout_cb_id,
                dialog_opened_cb_id
            );

            return read_bytes;
        }

        case static_cast<uint32_t>(StateId::Sleep): {
            uint32_t timeout_id = 0;
            float timeout_val = 0.f;
            uint32_t current_read = 0;
            bool success = true;

            current_read = a_serde->ReadRecordData(&timeout_id, sizeof(timeout_id));
            if (current_read != sizeof(timeout_id)) success = false; else read_bytes += current_read;
            current_read = a_serde->ReadRecordData(&timeout_val, sizeof(timeout_val));
            if (current_read != sizeof(timeout_val)) success = false; else read_bytes += current_read;

            if (!success) {
                logger::critical("LoadEventState state_sleep error");
                out_state = std::monostate{};
                return -1;
            }

            out_state = fsm::loader::create_state_sleep(
                active_chain,
                timeout_val,
                timeout_id
            );

            return read_bytes;
        }

        case static_cast<uint32_t>(StateId::Poll): {
            uint32_t poll_id = 0;
            uint32_t current_read = 0;
            bool success = true;

            current_read = a_serde->ReadRecordData(&poll_id, sizeof(poll_id));
            if (current_read != sizeof(poll_id)) success = false; else read_bytes += current_read;
          
            if (!success) {
                logger::critical("LoadEventState state_poll error");
                out_state = std::monostate{};
                return -1;
            }

            out_state = fsm::loader::create_state_poll(
                active_chain,
                poll_id
            );

            return read_bytes;
        }

        case static_cast<uint32_t>(StateId::SceneStart): {
            uint32_t timeout_id = 0;
            uint32_t serve_id = 0;
            float timeout_val = 0.f;
            float handle_val = 0.f;

            uint32_t current_read = 0;
            bool success = true;

            current_read = a_serde->ReadRecordData(&timeout_id, sizeof(timeout_id));
            if (current_read != sizeof(timeout_id)) success = false; else read_bytes += current_read;
            current_read = a_serde->ReadRecordData(&serve_id, sizeof(serve_id));
            if (current_read != sizeof(serve_id)) success = false; else read_bytes += current_read;

            current_read = a_serde->ReadRecordData(&timeout_val, sizeof(timeout_val));
            if (current_read != sizeof(timeout_val)) success = false; else read_bytes += current_read;
            current_read = a_serde->ReadRecordData(&handle_val, sizeof(handle_val));
            if (current_read != sizeof(handle_val)) success = false; else read_bytes += current_read;

            if (!success) {
                logger::critical("LoadEventState state_scene_start error");
                out_state = std::monostate{};
                return -1;
            }

            out_state = fsm::loader::create_state_scene_start(
                active_chain,
                timeout_val,
                handle_val,
                timeout_id,
                serve_id
            );

            return read_bytes;
        }

        case static_cast<uint32_t>(StateId::SceneEnd): {
            uint32_t timeout_id = 0;
            uint32_t papyrus_id = 0;
            uint32_t modevent_id = 0;
            float timeout = 0.f;
            float handle = 0.f;
            RE::BSFixedString event;

            uint32_t current_read = 0;
            bool success = true;

            current_read = a_serde->ReadRecordData(&timeout_id, sizeof(timeout_id));
            if (current_read != sizeof(timeout_id)) success = false; else read_bytes += current_read;
            current_read = a_serde->ReadRecordData(&papyrus_id, sizeof(papyrus_id));
            if (current_read != sizeof(papyrus_id)) success = false; else read_bytes += current_read;
            current_read = a_serde->ReadRecordData(&modevent_id, sizeof(modevent_id));
            if (current_read != sizeof(modevent_id)) success = false; else read_bytes += current_read;

            current_read = a_serde->ReadRecordData(&timeout, sizeof(timeout));
            if (current_read != sizeof(timeout)) success = false; else read_bytes += current_read;
            current_read = a_serde->ReadRecordData(&handle, sizeof(handle));
            if (current_read != sizeof(handle)) success = false; else read_bytes += current_read;
            current_read = LoadString(a_serde, event);
            if (current_read < 0) success = false; else read_bytes += current_read;

            if (!success) {
                logger::critical("LoadEventState state_scene_start error");
                out_state = std::monostate{};
                return -1;
            }

            out_state = fsm::loader::create_state_scene_end(
                active_chain,
                timeout,
                handle,
                event,
                timeout_id,
                papyrus_id,
                modevent_id
            );

            return read_bytes;
        }





        }
        out_state = std::monostate{};
        return -1;
    }

    template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

    inline bool SaveState(SKSE::SerializationInterface* a_serde, fsm::fsm_state& out_state) {
        // Beim Speichern:
        uint32_t state_type = static_cast<uint32_t>(out_state.index());
        if (!a_serde->WriteRecordData(&state_type, sizeof(state_type)))
            return false;

        bool success = std::visit(overloaded{
        [](std::monostate&) { logger::info("SaveState: std::monostate");  return true; },
        [a_serde](fsm::state_idle& s) { logger::info("SaveState: state_idle"); return true; },
        [a_serde](fsm::state_sleep& s) {
                return  a_serde->WriteRecordData(&s._timeout_p, sizeof(s._timeout_p));   // timeout_p
            },
        [a_serde](fsm::state_dialog_open& s) {
                logger::info("SaveState: state_dialog_open");
                return  a_serde->WriteRecordData(&s._timeout_p, sizeof(s._timeout_p))   // timeout_p
                    && serialization::SaveActorHandle(a_serde, s._speaker)              // speaker
                    && a_serde->WriteRecordData(&s.fn_timeout.first, sizeof(s.fn_timeout.first))         // fn_timeout
                    && a_serde->WriteRecordData(&s.fn_menu_open.first, sizeof(s.fn_menu_open.first));    // fn_menu_open
            },
        [a_serde](fsm::state_dialog_close& s) {
                return a_serde->WriteRecordData(&s.fn_topicinfo_begin.first, sizeof(s.fn_topicinfo_begin.first))  // fn_topicinfo_begin
                    && a_serde->WriteRecordData(&s.fn_menu_close.first, sizeof(s.fn_menu_close.first));           // fn_menu_close
            },
        [a_serde](fsm::state_poll& s) {
                 return a_serde->WriteRecordData(&s.fn_poll.first, sizeof(s.fn_poll.first));   // fn_poll
            },
        [a_serde](fsm::state_scene_start& s) {
                logger::info("SaveState: state_scene_start");
                return a_serde->WriteRecordData(&s._timeout_p, sizeof(s._timeout_p))   // timeout_p
                    && a_serde->WriteRecordData(&s._handle, sizeof(s._handle))         // handle
                    && a_serde->WriteRecordData(&s.fn_timeout.first, sizeof(s.fn_timeout.first))   // fn_timeout
                    && a_serde->WriteRecordData(&s.fn_papyrus.first, sizeof(s.fn_papyrus.first));  // fn_papyrus
            },
        [a_serde](fsm::state_scene_end& s) {
                logger::info("SaveState: state_scene_end");
                return a_serde->WriteRecordData(&s._timeout_p, sizeof(s._timeout_p))   // timeout_p
                    && a_serde->WriteRecordData(&s._handle, sizeof(s._handle))         // handle
                    && SaveString(a_serde, s._event_name)                              // event_name
                    && a_serde->WriteRecordData(&s.fn_timeout.first, sizeof(s.fn_timeout.first))     // fn_timeout
                    && a_serde->WriteRecordData(&s.fn_papyrus.first, sizeof(s.fn_papyrus.first))     // fn_papyrus
                    && a_serde->WriteRecordData(&s.fn_modevent.first, sizeof(s.fn_modevent.first));  // fn_modevent
            }
            }, out_state);

        return success;
    }

}
