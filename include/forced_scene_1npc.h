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
            constexpr uint32_t dialog_open_timeout_cb_id = 1;
            constexpr uint32_t dialog_open_success_cb_id = 2;
            constexpr uint32_t backhug_sleep1_cb_id = 3;
            constexpr uint32_t backhug_sleep2_cb_id = 4;
            constexpr uint32_t struggle_poll_cb_id = 5;
            constexpr uint32_t scene_start_timeout_cb_id = 6;
            constexpr uint32_t scene_start_success_cb_id = 7;
            constexpr uint32_t scene_end_timeout_cb_id = 8;
            constexpr uint32_t scene_end_papyrus_cb_id = 9;
            constexpr uint32_t scene_end_modevent_cb_id = 10;
            constexpr uint32_t dialog_close_menu_cb_id = 11;
            constexpr uint32_t dialog_close_topicinfo_cb_id = 12;
        }

        struct forced_scene_1npc : delayed {
            RE::ActorHandle _npc1_handle;
            std::optional<animation::backhug::backhug_data> _backhug_data;
            std::optional<sl_scenes::sl_scene_hook_names> _sl_hook_data;
			bool _set_away = false;

            bool trigger_phase1(game::context& ctx);
            std::optional<fsm::fsm_state> trigger_phase2(game::context& ctx);

            bool serialize(SKSE::SerializationInterface* a_serde, float a_current_time);
            int32_t deserialize(SKSE::SerializationInterface* a_serde);

            void reset_npc1_handle();
            std::optional<fsm::fsm_state>  scene_error_return(game::context& ctx);
            std::optional<fsm::fsm_state>  scene_error_return_xl(game::context& ctx);

            // Callbacks for fsm states
            std::optional<fsm::fsm_state> dialog_open_timeout_cb(game::context& ctx);
            std::optional<fsm::fsm_state> dialog_open_success_cb(game::context& ctx, RE::Actor* a_speaker);

            std::optional<fsm::fsm_state> dialog_close_topicinfo_cb(game::context& ctx, RE::Actor* a_speaker, RE::FormID a_form_id);
            std::optional<fsm::fsm_state> dialog_close_menu_cb(game::context& ctx);

            std::optional<fsm::fsm_state> backhug_sleep1_cb(game::context& ctx);
            std::optional<fsm::fsm_state> backhug_sleep2_cb(game::context& ctx);

            std::optional<fsm::fsm_state> struggle_poll_cb(game::context& ctx);

            std::optional<fsm::fsm_state> scene_start_timeout_cb(game::context& ctx);
            std::pair<bool, std::optional<fsm::fsm_state>> scene_start_success_cb(game::context& ctx, RE::TESQuest* a_this, float a_handle);

            std::optional<fsm::fsm_state> scene_end_timeout_cb(game::context& ctx);
            std::pair<bool, std::optional<fsm::fsm_state>> scene_end_papyrus_cb(game::context& ctx, float a_handle, int32_t a_thread_id);
            std::optional<fsm::fsm_state> scene_end_modevent_cb(game::context& ctx);

            template<typename Signature>
            static Signature resolve(uint32_t id) {
                using namespace forced_scene_1npc_ids;

                // SIGNATUR 1: Standard-Timeout (game::context&)
                if constexpr (std::is_same_v<Signature, std::optional<fsm::fsm_state>(forced_scene_1npc::*)(game::context&)>) {
                    switch (id) {
                    case dialog_open_timeout_cb_id: return &forced_scene_1npc::dialog_open_timeout_cb;
                    case backhug_sleep1_cb_id: return &forced_scene_1npc::backhug_sleep1_cb;
                    case backhug_sleep2_cb_id: return &forced_scene_1npc::backhug_sleep2_cb;
                    case struggle_poll_cb_id: return &forced_scene_1npc::struggle_poll_cb;
                    case scene_start_timeout_cb_id: return &forced_scene_1npc::scene_start_timeout_cb;
                    case scene_end_papyrus_cb_id: return &forced_scene_1npc::scene_end_timeout_cb;
                    case scene_end_modevent_cb_id: return &forced_scene_1npc::scene_end_modevent_cb;
                    default: return nullptr;
                    }
                }
                // SIGNATUR 2: Dialog Opened (game::context&, RE::Actor*)
                else if constexpr (std::is_same_v<Signature, std::optional<fsm::fsm_state>(forced_scene_1npc::*)(game::context&, RE::Actor*)>) {
                    switch (id) {
                    case dialog_open_success_cb_id: return &forced_scene_1npc::dialog_open_success_cb;
                    default: return nullptr;
                    }
                }
                // SIGNATUR 3:
                else if constexpr (std::is_same_v<Signature, std::pair<bool, std::optional<fsm::fsm_state>>(forced_scene_1npc::*)(game::context&, RE::TESQuest * a_this, float a_handle)>) {
                    switch (id) {
                    case scene_start_success_cb_id: return &forced_scene_1npc::scene_start_success_cb;
                    default: return nullptr;
                    }
                }
                // SIGNATUR 4:
                else if constexpr (std::is_same_v<Signature, std::pair<bool, std::optional<fsm::fsm_state>>(forced_scene_1npc::*)(game::context&, float a_handle, int32_t a_thread_id)>) {
                    switch (id) {
                    case scene_end_papyrus_cb_id: return &forced_scene_1npc::scene_end_papyrus_cb;
                    default: return nullptr;
                    }
                }

                return nullptr; // Unbekannte Signatur
            }


        };


   

   

    }
}
