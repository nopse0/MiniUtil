#pragma once

#include "fsm_loader.h"

namespace serialization {

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
        Idx_None = 0,
        Idx_Idle,
        Idx_Sleep,
        Idx_WaitDialogFinished,
        Idx_Poll,
        Idx_ServeSceneParameters,
        Idx_WaitSceneStarted,
        Idx_WaitForModEvent
    };

    template<typename ChainType>
    int32_t LoadState(ChainType* active_chain, SKSE::SerializationInterface* a_serde, fsm::fsm_state& out_state /* Oder aus Savegame lesen */) {

        // 1. Lies den State-Index/Typ aus dem Savegame (Simuliert)
        uint32_t loaded_state_type = 0;
        int32_t read_bytes = 0;
        read_bytes = a_serde->ReadRecordData(&loaded_state_type, sizeof(loaded_state_type));
        if (read_bytes != sizeof(loaded_state_type)) {
            logger::critical("Fehler beim Laden des FSM - Records: Unerwartetes Dateiende!");
            out_state = std::monostate{};
            return -1;
        }

        // 2. Der Switch bestimmt, WELCHE Daten jetzt aus dem Savegame gelesen werden müssen
        switch (loaded_state_type) {

        case static_cast<uint32_t>(StateId::Idx_None):
            out_state = std::monostate{};
            return read_bytes;

        case static_cast<uint32_t>(StateId::Idx_Idle):
            out_state = fsm::state_idle{};
            return read_bytes;

        case static_cast<uint32_t>(StateId::Idx_WaitDialogFinished): {
            uint32_t timeout_id = 0;
            uint32_t dialog_id = 0;
            float timeout_val = 0.f;

            uint32_t current_read = 0;
            bool success = true;

            current_read = a_serde->ReadRecordData(&timeout_id, sizeof(timeout_id));
            if (current_read != sizeof(timeout_id)) success = false;
            current_read = a_serde->ReadRecordData(&dialog_id, sizeof(dialog_id));
            if (current_read != sizeof(dialog_id)) success = false;
            current_read = a_serde->ReadRecordData(&timeout_val, sizeof(timeout_val));
            if (current_read != sizeof(timeout_val)) success = false;

            if (!success) {
                logger::critical("Fehler beim Laden des FSM - Records: Unerwartetes Dateiende!");
                out_state = std::monostate{};
                return -1;
            }

            out_state = fsm::loader::create_state_wait_dialog_finished(
                active_chain,
                timeout_val,
                timeout_id,
                dialog_id
            );

            return read_bytes + 12;
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
        [](std::monostate&) { return true; },
        [a_serde](fsm::state_idle& s) { return true; },
        [a_serde](fsm::state_sleep& s) { return false; },
        [a_serde](fsm::state_wait_dialog_finished& s) {
                return a_serde->WriteRecordData(&s.on_timeout.first, sizeof(s.on_timeout.first))
                    && a_serde->WriteRecordData(&s.on_dialog_finished.first, sizeof(s.on_dialog_finished.first))
                    && a_serde->WriteRecordData(&s._timeout_p, sizeof(s._timeout_p));
            },
        [a_serde](fsm::state_poll& s) { return false; },
        [a_serde](fsm::state_serve_scene_parameters& s) { return false; },
        [a_serde](fsm::state_wait_scene_started& s) { return false; },
        [a_serde](fsm::state_wait_for_mod_event& s) { return false; }
            }, out_state);

        return success;
    }

}
