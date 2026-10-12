#include <iostream>

#if 0
// --- Platzhalter für deine Engine-Klassen ---
struct GameEngine { void cancel_all_animations() {} };
struct HistoryStats { double get_trigger_chance() { return 0.85; } };
struct GameContext { GameEngine& engine; HistoryStats& stats; };

// --- Events ---
struct TimerTickEvent { GameContext& ctx; };
struct AbortEvent { GameContext& ctx; }; // Für erzwungenen Abbruch vor dem Speichern

// --- Zustände ---
struct StateIdleCheckingDelay {
    int current_delay_ticks = 0;

    // Dieser Zustand kann serialisiert werden
    template<class Archive> void serialize(Archive& ar) { ar(current_delay_ticks); }
};

struct StateIdleReadyToTrigger {};

struct StatePhase1Animation {
    int animation_id = 42;
    // Dieser Zustand läuft aktiv und KANN NICHT serialisiert werden (keine serialize-Methode)

    // RAII: Falls die Variante den Zustand abrupt wechselt, bricht der Destruktor die Animation ab
    void cleanup(GameContext& ctx) {
        ctx.engine.cancel_all_animations();
    }
};

// --- Compile-Time Check für Serialisierung (C++20 Concept) ---
template<typename T>
concept Serializable = requires(T t) {
    t.serialize(std::declval<int&>()); // Ersetze 'int' durch deine echte Speicher-Klasse
};

// Die FSM-Variante
using FsmState = std::variant<StateIdleCheckingDelay, StateIdleReadyToTrigger, StatePhase1Animation>;

// --- Helper für das Visit-Pattern ---
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

class EventStateMachine {
private:
    FsmState current_state = StateIdleCheckingDelay{};
    GameContext& context;

    // Hilfsfunktion, um beim Verlassen eines Zustands sauberes Cleanup aufzurufen
    void leave_current_state() {
        std::visit(overloaded{
            [this](StatePhase1Animation& s) { s.cleanup(context); },
            [](auto& text) {} // Andere Zustände brauchen kein Cleanup
            }, current_state);
    }

public:
    EventStateMachine(GameContext& ctx) : context(ctx) {}

    void handle_event(auto&& event) {
        // Wir ermitteln den Folgezustand
        FsmState next_state = std::visit(overloaded{
            // IDLE: Nutzt Statistiken aus dem Kontext
            [](StateIdleCheckingDelay& s, const TimerTickEvent& e) -> FsmState {
                s.current_delay_ticks++;
                if (s.current_delay_ticks >= 10) return StateIdleReadyToTrigger{};
                return s;
            },
            [](StateIdleReadyToTrigger& s, const TimerTickEvent& e) -> FsmState {
                // Zugriff auf historische Statistiken für Wahrscheinlichkeit
                if (e.ctx.stats.get_trigger_chance() > 0.8) {
                    return StatePhase1Animation{};
                }
                return StateIdleCheckingDelay{0};
            },
            // ACTIVE PHASE: Tick-Verarbeitung
            [](StatePhase1Animation& s, const TimerTickEvent& e) -> FsmState {
                // Normaler Ablauf...
                return s;
            },

            // --- ABSTURZ / ABBRUCH MANAGEMENT ---
            [](StatePhase1Animation& s, const AbortEvent& e) -> FsmState {
                s.cleanup(e.ctx); // Expliziter Abbruch der Animationen/AI
                return StateIdleCheckingDelay{0}; // Sicherer, speicherbarer Zustand
            },
            [](auto& s, const AbortEvent& e) -> FsmState {
                return s; // Alle speicherbaren Zustände ignorieren den Abbruch
            }
            }, current_state, std::forward<decltype(event)>(event));

        // Falls sich der Zustand geändert hat, rufen wir ggf. Destruktoren/Cleanup auf
        if (next_state.index() != current_state.index()) {
            leave_current_state();
            current_state = std::move(next_state);
        }
    }

    // --- Speichern-Funktion ---
    bool try_save() {
        // Überprüfen, ob der aktuelle Zustand im std::variant das Concept erfüllt
        bool can_save = std::visit([](auto& s) {
            return Serializable<decltype(s)>;
            }, current_state);

        if (!can_save) {
            std::cout << "Zustand nicht serialisierbar! Erzwinge Abbruch...\n";
            handle_event(AbortEvent{ context }); // Holt die FSM zurück in die IDLE-Phase
        }

        // Jetzt ist der Zustand garantiert serialisierbar
        std::cout << "Speichern erfolgreich durchgeführt.\n";
        return true;
    }
};
#endif


// --- Platzhalter für deine Engine-Klassen ---
struct GameEngine { void cancel_all_animations() {} };
struct HistoryStats { double get_trigger_chance() { return 0.85; } };
struct GameContext { GameEngine& engine; HistoryStats& stats; };

// --- Events ---
struct TimerTickEvent { GameContext& ctx; };
struct AbortEvent { GameContext& ctx; }; // Für erzwungenen Abbruch vor dem Speichern

// --- Zustände ---
struct StateIdleCheckingDelay {
    int current_delay_ticks = 0;

    // Dieser Zustand kann serialisiert werden
    template<class Archive> void serialize(Archive& ar) { ar(current_delay_ticks); }
};

struct StateIdleReadyToTrigger {};

struct StatePhase1Animation {
    int animation_id = 42;
    // Dieser Zustand läuft aktiv und KANN NICHT serialisiert werden (keine serialize-Methode)

    // RAII: Falls die Variante den Zustand abrupt wechselt, bricht der Destruktor die Animation ab
    void cleanup(GameContext& ctx) {
        ctx.engine.cancel_all_animations();
    }
};

// --- Compile-Time Check für Serialisierung (C++20 Concept) ---
template<typename T>
concept Serializable = requires(T t) {
    t.serialize(std::declval<int&>()); // Ersetze 'int' durch deine echte Speicher-Klasse
};

// Die FSM-Variante
using FsmState = std::variant<StateIdleCheckingDelay, StateIdleReadyToTrigger, StatePhase1Animation>;

// --- Helper für das Visit-Pattern ---
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

class EventStateMachine {
private:
    FsmState current_state = StateIdleCheckingDelay{};
    GameContext& context;

    // Hilfsfunktion, um beim Verlassen eines Zustands sauberes Cleanup aufzurufen
    void leave_current_state() {
        std::visit(overloaded{
            [this](StatePhase1Animation& s) { s.cleanup(context); },
            [](auto& text) {} // Andere Zustände brauchen kein Cleanup
            }, current_state);
    }

public:
    EventStateMachine(GameContext& ctx) : context(ctx) {}

    void handle_event(auto&& event) {
        // Wir übergeben NUR current_state an std::visit. 
        // Das konkrete 'event' fangen wir per perfect forwarding im Lambda ab.
        FsmState next_state = std::visit([&](auto& s) -> FsmState {

            // --- Logik für TimerTickEvent ---
            if constexpr (std::is_same_v<std::decay_t<decltype(event)>, TimerTickEvent>) {
                if constexpr (std::is_same_v<std::decay_t<decltype(s)>, StateIdleCheckingDelay>) {
                    s.current_delay_ticks++;
                    if (s.current_delay_ticks >= 10) return StateIdleReadyToTrigger{};
                    return s;
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(s)>, StateIdleReadyToTrigger>) {
                    if (event.ctx.stats.get_trigger_chance() > 0.8) {
                        return StatePhase1Animation{};
                    }
                    return StateIdleCheckingDelay{ 0 };
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(s)>, StatePhase1Animation>) {
                    // Normaler Tick-Ablauf in Phase 1
                    return s;
                }
            }

            // --- Logik für AbortEvent ---
            else if constexpr (std::is_same_v<std::decay_t<decltype(event)>, AbortEvent>) {
                if constexpr (std::is_same_v<std::decay_t<decltype(s)>, StatePhase1Animation>) {
                    s.cleanup(event.ctx); // Animation stoppen
                    return StateIdleCheckingDelay{ 0 }; // Zurück in sicheren Zustand
                }
                return s; // Alle anderen Zustände ignorieren den Abbruch
            }

            return s; // Fallback

            }, current_state);

        // Zustand wechseln und ggf. aufräumen
        if (next_state.index() != current_state.index()) {
            leave_current_state();
            current_state = std::move(next_state);
        }
    }

    // --- Speichern-Funktion ---
    bool try_save() {
        // Überprüfen, ob der aktuelle Zustand im std::variant das Concept erfüllt
        bool can_save = std::visit([](auto& s) {
            return Serializable<decltype(s)>;
            }, current_state);

        if (!can_save) {
            std::cout << "Zustand nicht serialisierbar! Erzwinge Abbruch...\n";
            handle_event(AbortEvent{ context }); // Holt die FSM zurück in die IDLE-Phase
        }

        // Jetzt ist der Zustand garantiert serialisierbar
        std::cout << "Speichern erfolgreich durchgeführt.\n";
        return true;
    }
};


