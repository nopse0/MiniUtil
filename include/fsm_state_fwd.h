#pragma once

namespace fsm {
    // 1. Forward-declare every state struct
    struct state_idle;
    struct state_force_greet_1npc;

    // 2. Define the variant type alias
    // This is 100% legal because C++ allows a std::variant to hold pointers 
    // or be defined using forward-declared structs in a type alias.
    using fsm_state = std::variant<
        state_idle,
        state_force_greet_1npc
    >;
}
