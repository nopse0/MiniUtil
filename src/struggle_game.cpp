#include "minigame.h"

namespace minigame {

    void struggle_game::start(int a_timeout_seconds) {
        _timeout_duration = std::chrono::seconds(a_timeout_seconds);
        _progress.store(0.0f, std::memory_order_relaxed);
        _start_time = std::chrono::steady_clock::now();

        _state.store(state::playing, std::memory_order_release);
    }

    void struggle_game::reset() {
        _state.store(state::idle, std::memory_order_relaxed);
    }

    bool struggle_game::on_input(RE::InputEvent* a_event) {
        if (_state.load(std::memory_order_relaxed) != state::playing) {
            return false;
        }
        if (a_event->device == RE::INPUT_DEVICE::kKeyboard) {
            if (auto button = a_event->AsButtonEvent()) {
                if ((button->GetIDCode() == RE::BSWin32KeyboardDevice::Key::kA) || (button->GetIDCode() == RE::BSWin32KeyboardDevice::Key::kD)) {
                    if (button->IsDown()) {
                        float current = _progress.load(std::memory_order_relaxed);
                        float next = std::min(current + 0.05f, 1.0f); // Add 5% per mash
                        _progress.store(next, std::memory_order_relaxed);
                        if (next >= 1.0f) {
                            _state.store(state::won, std::memory_order_relaxed);
                        }
                    }
                }
				// eat up all keyboard events, so the player isn't able to save/load/return to main menu
                return true;
            }
        }
		return false;
    }

    struggle_game::state struggle_game::get_state() {
        return _state.load(std::memory_order_acquire);
    }

    void struggle_game::render() {
        state current_state = _state.load(std::memory_order_acquire);

        if (current_state == state::idle) {
            return;
        }

        float progress = _progress.load(std::memory_order_relaxed);
        float time_used = 0.0f;

        if (current_state == state::playing) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - _start_time
            );

            time_used = static_cast<float>(elapsed.count()) / _timeout_duration.count();

            // Check if time ran out
            if (elapsed >= _timeout_duration) {
                _state.store(state::lost, std::memory_order_relaxed);
                current_state = state::lost;
            }
            else {
                float drained = std::max(progress - 0.002f, 0.0f); // Adjust drain speed
                _progress.store(drained, std::memory_order_relaxed);
                progress = drained;
            }
        }

        // --- Transparent, frameless window with centered large progress bar ---
        // Window flags: no decoration, no background so the window is visually transparent.
        ImGuiMCP::ImGuiWindowFlags window_flags = ImGuiMCP::ImGuiWindowFlags_NoDecoration
            | ImGuiMCP::ImGuiWindowFlags_NoBackground
            | ImGuiMCP::ImGuiWindowFlags_NoMove
            | ImGuiMCP::ImGuiWindowFlags_NoSavedSettings
            | ImGuiMCP::ImGuiWindowFlags_NoFocusOnAppearing;

        // Get display size to compute position/size (centered horizontally, lower half vertically).
        auto io = ImGuiMCP::GetIO();
        ImGuiMCP::ImVec2 display = ImGuiMCP::ImVec2(io->DisplaySize.x, io->DisplaySize.y);

        // Bar size: large and wide. Tweak multipliers to taste.
        const float bar_width = display.x * 0.60f;          // 60% of screen width
        const float bar_height = std::max(40.0f, display.y * 0.06f); // at least 40px or 6% of height

        // Position: centered horizontally, positioned in lower half (around 66% down)
        const float pos_x = (display.x - bar_width) * 0.5f;
        const float pos_y = display.y * 0.66f - (bar_height * 0.5f);

        ImGuiMCP::SetNextWindowPos(ImGuiMCP::ImVec2(pos_x, pos_y), ImGuiMCP::ImGuiCond_Always);
        ImGuiMCP::SetNextWindowSize(ImGuiMCP::ImVec2(bar_width, bar_height), ImGuiMCP::ImGuiCond_Always);
        ImGuiMCP::SetNextWindowBgAlpha(0.0f); // ensure fully transparent background

        ImGuiMCP::PushStyleVar(ImGuiMCP::ImGuiStyleVar_WindowPadding, ImGuiMCP::ImVec2(0.0f, 0.0f));
        ImGuiMCP::PushStyleVar(ImGuiMCP::ImGuiStyleVar_FrameRounding, 6.0f);
        ImGuiMCP::PushStyleVar(ImGuiMCP::ImGuiStyleVar_FramePadding, ImGuiMCP::ImVec2(6.0f, 6.0f));

        // Make the frame background fully transparent so only the filled bar is visible.
        ImGuiMCP::PushStyleVar(ImGuiMCP::ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_FrameBg, ImGuiMCP::ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        // Color of the progress (dynamic from green to red based on time remaining)
        ImGuiMCP::ImVec4 bar_color = ImGuiMCP::ImVec4(time_used, 1.0f - time_used, 0.0f, 0.95f);
        ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_PlotHistogram, bar_color);
        ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_PlotHistogramHovered, bar_color);
        //ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_PlotHistogramActive, bar_color);

        ImGuiMCP::Begin("Key Mash Challenge!", nullptr, window_flags);

        if (current_state == state::playing) {
            // The ProgressBar will use full available width. Height is given explicitly to make it big.
            ImGuiMCP::ProgressBar(progress, ImGuiMCP::ImVec2(-1.0f, bar_height - 12.0f)); // subtract padding for visual fit

            // Optionally show remaining time small and subtle above/below the bar:
            ImGuiMCP::BeginChild("##timeinfo", ImGuiMCP::ImVec2(0, 0), false, ImGuiMCP::ImGuiWindowFlags_NoBackground);
            ImGuiMCP::SetCursorPosY(ImGuiMCP::GetCursorPosY() + 4.0f); // small offset
            ImGuiMCP::TextColored(ImGuiMCP::ImVec4(1, 1, 1, 0.75f), "Time: %.1fs", (_timeout_duration.count() * (1.0f - time_used)) / 1000.0f);
            ImGuiMCP::EndChild();
        }

        ImGuiMCP::End();

        ImGuiMCP::PopStyleColor(3);
        ImGuiMCP::PopStyleVar(4);
    }


}
