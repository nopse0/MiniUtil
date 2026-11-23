#pragma once

namespace game {
    struct context;
}

namespace fsm {
    // 1. Forward-declare every state struct
    struct state_idle;
    struct state_sleep;
    struct state_dialog_open;
    struct state_dialog_close;
    struct state_poll;
    struct state_scene_start;
    //struct state_wait_scene_set_thread_id;
    struct state_scene_end;

    // 2. Define the variant type alias
    // This is 100% legal because C++ allows a std::variant to hold pointers 
    // or be defined using forward-declared structs in a type alias.
    using fsm_state = std::variant<
        std::monostate,
        state_idle,
        state_sleep,
        state_dialog_open,
        state_dialog_close,
        state_poll,
        state_scene_start,
        //state_wait_scene_set_thread_id,
        state_scene_end
    >;

}
