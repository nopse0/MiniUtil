#pragma once

class stats_with_half_life {
private:
    double _alpha;
    double _current_average = 0.0;
    double _last_tick_game_day = 0.0;
    double _k_constant;

public:
    inline stats_with_half_life(double half_life_game_days, double target_nominal_load)
        : _k_constant(target_nominal_load) {
        _alpha = std::log(2.0) / half_life_game_days;
    }

    // Call on brand new game / cold zero start
    inline void reset(double current_game_day) {
        _last_tick_game_day = current_game_day;
        _current_average = 0.0;
    }

    // Call when loading a save file
    inline void restore(double saved_average, double saved_game_day, double current_game_day) {
        _current_average = saved_average;
        _last_tick_game_day = saved_game_day;
        decay_to_present(current_game_day);
    }

    inline void record(double current_game_day, double value = 1.0) {
        decay_to_present(current_game_day);
        _current_average += value;
    }

    inline double get_average(double current_game_day) {
        decay_to_present(current_game_day);
        return _current_average;
    }

    inline double get_score(double current_game_day) {
        double avg = get_average(current_game_day);
        if (avg <= 0.0) return 0.0;
        return avg / (_k_constant + avg);
    }

    inline double get_last_stored_average_for_serialization() const { return _current_average; }
    inline double get_last_stored_game_day_for_serialization() const { return _last_tick_game_day; }

private:
    inline void decay_to_present(double current_game_day) {
        if (current_game_day <= _last_tick_game_day) return;

        double dt = current_game_day - _last_tick_game_day;
        _current_average = _current_average * std::exp(-_alpha * dt);
        _last_tick_game_day = current_game_day;
    }
};
