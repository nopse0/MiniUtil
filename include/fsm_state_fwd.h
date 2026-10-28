#pragma once

namespace game {
    struct context;
}

namespace fsm {
    // 1. Forward-declare every state struct
    struct state_idle;
    struct state_sleep;
    struct state_wait_dialog_finished;
    struct state_poll;
    struct state_serve_scene_parameters;
    struct state_wait_scene_set_thread_id;
    struct state_wait_for_mod_event;

    // 2. Define the variant type alias
    // This is 100% legal because C++ allows a std::variant to hold pointers 
    // or be defined using forward-declared structs in a type alias.
    using fsm_state = std::variant<
        std::monostate,
        state_idle,
        state_sleep,
        state_wait_dialog_finished,
        state_poll,
        state_serve_scene_parameters,
        state_wait_scene_set_thread_id,
        state_wait_for_mod_event
    >;

    //struct state_base {
    //    std::function<std::optional<fsm_state>(game::context& a_context)> on_pre_save;

    //    state_base(std::function<std::optional<fsm_state>(game::context& a_context)> pre_save_cb) : on_pre_save(std::move(pre_save_cb)) {}

    //    std::optional<fsm_state> pre_save(game::context& a_ctx) {
    //        return on_pre_save(a_ctx);
    //    }
    //};

}
