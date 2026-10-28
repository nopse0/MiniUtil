#pragma once

#include "event_chain_base.h"
#include "fsm_state_fwd.h"
#include "backhug.h"

// cycle here, need forward declaration
namespace game {
	struct context;
}
// cycle here, need forward declaration
namespace fsm {
	class fsm;
}

namespace event_chain {

    namespace forced_scene_1npc {

        namespace forced_scene_1npc_ids {
            constexpr uint32_t none = 0;
            constexpr uint32_t wait_dialog_finished_timeout_id = 1;
            constexpr uint32_t wait_dialog_finished_success_id = 2;
            constexpr uint32_t backhug_sleep1_id = 3;
            constexpr uint32_t backhug_sleep2_id = 4;
            constexpr uint32_t poll_struggle_game_id = 5;
            constexpr uint32_t serve_scene_parameters_timeout_id = 6;
            constexpr uint32_t serve_scene_parameters_serve_id = 7;
            constexpr uint32_t set_scene_thread_id_timeout_id = 8;
            constexpr uint32_t set_scene_thread_id_set_id = 9;
            constexpr uint32_t wait_for_mod_event_timeout_id = 10;
            constexpr uint32_t wait_for_mod_event_received_id = 11;
        }

        struct forced_scene_1npc : delayed {
            RE::ActorHandle _npc1_handle;
            std::optional<animation::backhug::backhug_data> _backhug_data;
            std::optional<sl_scenes::sl_scene_hook_names> _sl_hook_data;

            bool trigger_phase1(game::context& ctx);
            std::optional<fsm::fsm_state> trigger_phase2(game::context& ctx);

            bool serialize(SKSE::SerializationInterface* a_serde, float a_current_time);
            int32_t deserialize(SKSE::SerializationInterface* a_serde);

            std::optional<fsm::fsm_state>  scene_error_return(game::context& ctx);
            std::optional<fsm::fsm_state>  scene_error_return_xl(game::context& ctx);

            // Callbacks for fsm states
            std::optional<fsm::fsm_state> wait_dialog_finished_timeout_cb(game::context& ctx);
            std::optional<fsm::fsm_state> wait_dialog_finished_success_cb(game::context& ctx, RE::Actor* a_speaker);

            std::optional<fsm::fsm_state> backhug_sleep1_cb(game::context& ctx);
            std::optional<fsm::fsm_state> backhug_sleep2_cb(game::context& ctx);

            std::optional<fsm::fsm_state> poll_struggle_game_cb(game::context& ctx);

            std::optional<fsm::fsm_state> serve_scene_parameters_timeout_cb(game::context& ctx);
            std::pair<bool, std::optional<fsm::fsm_state>> serve_scene_parameters_serve_cb(game::context& ctx, RE::TESQuest* a_this, float a_handle);

            std::optional<fsm::fsm_state> set_scene_thread_id_timeout_cb(game::context& ctx);
            std::pair<bool, std::optional<fsm::fsm_state>> set_scene_thread_id_set_cb(game::context& ctx, float a_handle, int32_t a_thread_id);

            std::optional<fsm::fsm_state> wait_for_mod_event_timeout_cb(game::context& ctx);
            std::optional<fsm::fsm_state> wait_for_mod_event_received_cb(game::context& ctx);

            template<typename Signature>
            static Signature resolve(uint32_t id) {
                using namespace forced_scene_1npc_ids;

                // SIGNATUR 1: Standard-Timeout (game::context&)
                if constexpr (std::is_same_v<Signature, std::optional<fsm::fsm_state>(forced_scene_1npc::*)(game::context&)>) {
                    switch (id) {
                    case wait_dialog_finished_timeout_id: return &forced_scene_1npc::wait_dialog_finished_timeout_cb;
                    default: return nullptr;
                    }
                }
                // SIGNATUR 2: Dialog Finished (game::context&, RE::Actor*)
                else if constexpr (std::is_same_v<Signature, std::optional<fsm::fsm_state>(forced_scene_1npc::*)(game::context&, RE::Actor*)>) {
                    switch (id) {
                    case wait_dialog_finished_success_id: return &forced_scene_1npc::wait_dialog_finished_success_cb;
                    default: return nullptr;
                    }
                }

                return nullptr; // Unbekannte Signatur
            }


        };


   

   

    }
}
