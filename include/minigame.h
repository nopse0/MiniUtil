#pragma once

namespace minigame {
    class struggle_game : public singleton<struggle_game> {
    public:
        enum class state {
            idle,
            playing,
            won,
            lost
        };

        std::atomic<state> _state{ state::idle };
        std::atomic<float> _progress{ 0.0f }; // 0.0f to 1.0f

        std::chrono::steady_clock::time_point _start_time;
        std::chrono::milliseconds _timeout_duration{ 5000 }; // 5 seconds timeout

        state get_state();
        void start(int a_timeout_seconds = 5);
        bool on_input(RE::InputEvent* a_event);
        void render();
    };
}
