#pragma once

// std::variant problems again. This header needs all full fsm state definitions, so it can only be included after them.

namespace fsm {

	namespace loader {

        template<typename ChainType>
        state_wait_dialog_finished create_state_wait_dialog_finished(
            ChainType* chain,
            float timeout_val,
            uint32_t timeout_id,
            uint32_t dialog_finished_id)
        {
            // 1. Die Callbacks direkt über dein bind_callback Template auflösen
            // Keine manuellen Args... mehr nötig!
            auto timeout_cb = bind_callback<
                std::optional<::fsm::fsm_state>(ChainType::*)(game::context&)
            >(timeout_id, chain);

            auto dialog_cb = bind_callback<
                std::optional<::fsm::fsm_state>(ChainType::*)(game::context&, RE::Actor*)
            >(dialog_finished_id, chain);

            // 2. Den echten Custom-Konstruktor aufrufen und das fertige Objekt zurückgeben!
            return state_wait_dialog_finished(timeout_val, std::move(timeout_cb), std::move(dialog_cb));
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
        state_serve_scene_parameters create_state_serve_scene_parameters(
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
            return state_serve_scene_parameters(timeout_val, handle_val, std::move(timeout_cb), std::move(serve_cb));
        }

        template<typename ChainType>
        state_wait_scene_set_thread_id create_state_wait_scene_set_thread_id(
            ChainType* chain,
            float timeout_val,
            float handle_val,
            uint32_t timeout_id,
            uint32_t setter_id)
        {
            // 1. Die Callbacks direkt über dein bind_callback Template auflösen
            auto timeout_cb = bind_callback<
                std::optional<::fsm::fsm_state>(ChainType::*)(game::context&)
            >(timeout_id, chain);

            auto setter_cb = bind_callback<
                std::pair<bool, std::optional<::fsm::fsm_state>>(ChainType::*)(game::context&, float, int32_t)
            >(setter_id, chain);

            // 2. Den echten Custom-Konstruktor aufrufen und das fertige Objekt zurückgeben!
            return state_wait_scene_set_thread_id(timeout_val, handle_val, std::move(timeout_cb), std::move(setter_cb));
        }

        template<typename ChainType>
        state_wait_for_mod_event create_state_wait_for_mod_event(
            ChainType* chain,
            float timeout_val,
            uint32_t timeout_id,
            uint32_t received_id)
        {
            // 1. Die Callbacks direkt über dein bind_callback Template auflösen
            // Keine manuellen Args... mehr nötig!
            auto timeout_cb = bind_callback<
                std::optional<::fsm::fsm_state>(ChainType::*)(game::context&)
            >(timeout_id, chain);

            auto received_cb = bind_callback<
                std::optional<::fsm::fsm_state>(ChainType::*)(game::context&)
            >(received_id, chain);

            // 2. Den echten Custom-Konstruktor aufrufen und das fertige Objekt zurückgeben!
            return state_wait_for_mod_event(timeout_val, std::move(timeout_cb), std::move(received_cb));
        }


	}
}
