#pragma once

// std::variant problems again. This header needs all full fsm state definitions, so it can only be included after them.

namespace fsm {

	namespace loader {

        template<typename ChainType>
        state_dialog_open create_state_dialog_open(
            ChainType* chain,
            float timeout_val,
            const RE::ActorHandle& speaker,
            uint32_t timeout_cb_id,
            uint32_t dialog_opened_cb_id)
        {
            // 1. Die Callbacks direkt über dein bind_callback Template auflösen
            // Keine manuellen Args... mehr nötig!
            auto timeout_cb = bind_callback<
                std::optional<::fsm::fsm_state>(ChainType::*)(game::context&)
            >(timeout_cb_id, chain);

            auto dialog_cb = bind_callback<
                std::optional<::fsm::fsm_state>(ChainType::*)(game::context&, RE::Actor*)
            >(dialog_opened_cb_id, chain);

            // 2. Den echten Custom-Konstruktor aufrufen und das fertige Objekt zurückgeben!
            return state_dialog_open(timeout_val, speaker, std::move(timeout_cb), std::move(dialog_cb));
        }


        template<typename ChainType>
        state_sleep create_state_sleep(
            ChainType* chain,
            float timeout_val,
            uint32_t timeout_id)
        {
            auto timeout_cb = bind_callback<
                std::optional<::fsm::fsm_state>(ChainType::*)(game::context&)
            >(timeout_id, chain);

            return state_sleep(timeout_val, std::move(timeout_cb));
        }
        
        
        template<typename ChainType>
        state_poll create_state_poll(
            ChainType* chain,
            uint32_t poll_id)
        {
            auto poll_cb = bind_callback<
                std::optional<::fsm::fsm_state>(ChainType::*)(game::context&)
            >(poll_id, chain);

            return state_poll(std::move(poll_cb));
        }

        template<typename ChainType>
        state_scene_start create_state_scene_start(
            ChainType* chain,
            float timeout_val,
            float handle_val,
            uint32_t timeout_id,
            uint32_t serve_id)
        {
            // 1. Die Callbacks direkt über dein bind_callback Template auflösen
            auto timeout_cb = bind_callback<
                std::optional<::fsm::fsm_state>(ChainType::*)(game::context&)
            >(timeout_id, chain);

            auto serve_cb = bind_callback<
                std::pair<bool,std::optional<::fsm::fsm_state>>(ChainType::*)(game::context&, RE::TESQuest*, float)
            >(serve_id, chain);

            // 2. Den echten Custom-Konstruktor aufrufen und das fertige Objekt zurückgeben!
            return state_scene_start(timeout_val, handle_val, std::move(timeout_cb), std::move(serve_cb));
        }

        template<typename ChainType>
        state_scene_end create_state_scene_end(
            ChainType* chain,
            float a_timeout,
            float a_handle,
            RE::BSFixedString a_event,
            uint32_t timeout_cb_id,
            uint32_t papyrus_cb_id,
            uint32_t modevent_cb_id)
        {
            // 1. Die Callbacks direkt über dein bind_callback Template auflösen
            // Keine manuellen Args... mehr nötig!
            auto timeout_cb = bind_callback<
                std::optional<::fsm::fsm_state>(ChainType::*)(game::context&)
            >(timeout_cb_id, chain);

            auto papyrus_cb = bind_callback<
                std::pair<bool, std::optional<::fsm::fsm_state>>(ChainType::*)(game::context&, float a_handle, int32_t a_thread_id)
            >(papyrus_cb_id, chain);

            auto modevent_cb = bind_callback<
                std::optional<::fsm::fsm_state>(ChainType::*)(game::context&)
            >(modevent_cb_id, chain);

            // 2. Den echten Custom-Konstruktor aufrufen und das fertige Objekt zurückgeben!
            return state_scene_end(a_timeout, a_handle, a_event, std::move(timeout_cb), std::move(papyrus_cb), std::move(modevent_cb));
        }

	}
}
