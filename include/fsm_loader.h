#pragma once

// std::variant problems again. This header needs all full fsm state definitions, so it can only be included after them.

namespace fsm {

	namespace loader {

        // Universelle Fabrik für 'state_wait_dialog_finished'
        template<typename ChainType>
        state_wait_dialog_finished create_state_wait_dialog_finished(
            ChainType* chain,
            float timeout_val,
            uint32_t timeout_id,
            uint32_t dialog_finished_id)
        {
            // 1. Die Callbacks direkt über dein bind_callback Template auflösen
            auto timeout_cb = bind_callback<
                std::optional<::fsm::fsm_state>(ChainType::*)(game::context&)
            >(timeout_id, chain);

            auto dialog_cb = bind_callback<
                std::optional<::fsm::fsm_state>(ChainType::*)(game::context&, RE::Actor*),
                ChainType,
                RE::Actor*
            >(dialog_finished_id, chain);

            // 2. Den echten Custom-Konstruktor aufrufen und das fertige Objekt zurückgeben!
            return state_wait_dialog_finished(timeout_val, std::move(timeout_cb), std::move(dialog_cb));
        }


	}
}
