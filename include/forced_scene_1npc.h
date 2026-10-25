#pragma once

#include "event_chain_base.h"
#include "fsm_state_fwd.h"

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
            constexpr uint32_t timeout = 1;
            constexpr uint32_t dialog_finished = 2;
        }

        struct forced_scene_1npc : delayed {
            RE::ActorHandle _npc1_handle;

            bool trigger_phase1(game::context& ctx);
            std::optional<fsm::fsm_state> trigger_phase2(game::context& ctx);

            bool serialize(SKSE::SerializationInterface* a_serde);
            int32_t deserialize(SKSE::SerializationInterface* a_serde);

            // Callbacks for fsm states
            std::optional<fsm::fsm_state> wait_dialog_finished_timeout_cb(game::context& ctx);
            std::optional<fsm::fsm_state> wait_dialog_finished_finished_cb(game::context& ctx, RE::Actor* a_speaker);


            template<typename Signature>
            static Signature resolve(uint32_t id) {
                using namespace forced_scene_1npc_ids;

                // SIGNATUR 1: Standard-Timeout (game::context&)
                if constexpr (std::is_same_v<Signature, std::optional<fsm::fsm_state>(forced_scene_1npc::*)(game::context&)>) {
                    switch (id) {
                    case timeout: return &forced_scene_1npc::wait_dialog_finished_timeout_cb;
                    default: return nullptr;
                    }
                }
                // SIGNATUR 2: Dialog Finished (game::context&, RE::Actor*)
                else if constexpr (std::is_same_v<Signature, std::optional<fsm::fsm_state>(forced_scene_1npc::*)(game::context&, RE::Actor*)>) {
                    switch (id) {
                    case dialog_finished: return &forced_scene_1npc::wait_dialog_finished_finished_cb;
                    default: return nullptr;
                    }
                }

                return nullptr; // Unbekannte Signatur
            }


        };


   

   

    }
}
