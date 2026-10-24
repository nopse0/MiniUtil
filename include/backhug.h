#pragma once

namespace animation {

    struct backhug {
        static constexpr float _sleep1_duration_seconds = 0.5f;
        static constexpr float _sleep2_duration_seconds = 0.5f;

        RE::ActorHandle _hugger_handle;
        std::function<void()> _sleep1;
        std::function<void()> _sleep2;

        bool start(RE::Actor* a_actor);
        void stop();
    };

}
