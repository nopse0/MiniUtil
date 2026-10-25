#include "forced_scene_1npc.h"
#include "game_context.h"
#include "form_cache.h"
#include "actor_util.h"
#include "state_all.h"
#include "fsm.h"
#include "scanner.h"
#include "backhug.h"
#include "skyrim_util.h"
#include "stat_manager.h"
#include "random.h"
#include "fsm_loader.h"

namespace event_chain {
    bool delayed::serialize(SKSE::SerializationInterface* a_serde) {
        uint8_t active = _is_active;
        return a_serde->WriteRecordData(&_next_play_second, sizeof(_next_play_second))
            && a_serde->WriteRecordData(&_next_game_day, sizeof(_next_game_day))
            && a_serde->WriteRecordData(&_delay_play_seconds, sizeof(_delay_play_seconds))
            && a_serde->WriteRecordData(&_delay_game_days, sizeof(_delay_game_days))
            && a_serde->WriteRecordData(&_delay_retry_play_seconds, sizeof(_delay_retry_play_seconds))
            && a_serde->WriteRecordData(&active, sizeof(active));
    }

    int32_t delayed::deserialize(SKSE::SerializationInterface* a_serde) {
        int32_t read_bytes = 0;
        uint32_t current_read = 0;

        current_read = a_serde->ReadRecordData(&_next_play_second, sizeof(_next_play_second));
        if (current_read != sizeof(_next_play_second)) return -1; else read_bytes += current_read;
        current_read = a_serde->ReadRecordData(&_next_game_day, sizeof(_next_game_day));
        if (current_read != sizeof(_next_game_day)) return -1; else read_bytes += current_read;
        current_read = a_serde->ReadRecordData(&_delay_play_seconds, sizeof(_delay_play_seconds));
        if (current_read != sizeof(_delay_play_seconds)) return -1; else read_bytes += current_read;
        current_read = a_serde->ReadRecordData(&_delay_game_days, sizeof(_delay_game_days));
        if (current_read != sizeof(_delay_game_days)) return -1; else read_bytes += current_read;
        current_read = a_serde->ReadRecordData(&_delay_retry_play_seconds, sizeof(_delay_retry_play_seconds));
        if (current_read != sizeof(_delay_retry_play_seconds)) return -1; else read_bytes += current_read;
        uint8_t active = false;
        current_read = a_serde->ReadRecordData(&active, sizeof(active));
        if (current_read != sizeof(active)) return -1; else read_bytes += current_read;
        _is_active = active;

        return read_bytes;
    }

    namespace forced_scene_1npc {
        bool forced_scene_1npc::serialize(SKSE::SerializationInterface* a_serde) {
            if (!delayed::serialize(a_serde))
                return false;

            RE::FormID formId = 0;
            auto actorPtr = _npc1_handle.get();
            if (actorPtr) {
                formId = actorPtr->GetFormID();
            }
            return a_serde->WriteRecordData(&formId, sizeof(formId));
        }

        int32_t forced_scene_1npc::deserialize(SKSE::SerializationInterface* a_serde) {
            int32_t read_bytes = 0;
            uint32_t current_read = 0;

            read_bytes = delayed::deserialize(a_serde);
            if (read_bytes < 0) return -1;

            RE::FormID savedFormID = 0;     // Lot of boiler-plate code for serializing actor handles
            current_read = a_serde->ReadRecordData(&savedFormID, sizeof(savedFormID));
            if (current_read != sizeof(savedFormID)) return -1; else read_bytes += current_read;
            // Relocate FormID
            if (savedFormID != 0) {
                RE::FormID resolvedFormID = 0;
                // CRITICAL: SKSE korrigiert hier die FormID basierend auf der aktuellen Load-Order!
                if (a_serde->ResolveFormID(savedFormID, resolvedFormID)) {
                    // Hole das Objekt über die korrigierte FormID
                    auto refr = RE::TESForm::LookupByID<RE::Actor>(resolvedFormID);
                    if (refr) {
                        // Erstelle wieder dein sicheres ObjectRefHandle für deine Klasse
                        _npc1_handle = refr->GetHandle();
                    }
                }
            }
            return read_bytes;
        }

        bool forced_scene_1npc::trigger_phase1(game::context& ctx) {
            logger_custom::debug("forced_scene_1npc::trigger_phase1");

            // quick check
            if (_next_game_day > ctx._clock.get_game_day()) {
                logger_custom::debug("forced_scene_1npc::trigger_phase1 game day cooldown still active");
                return false;  // fsm calls update_cooldown if trigger_phase1 returns false, don't have to do this here
            }

            // expensive check
            std::vector< std::pair<float, RE::Actor*> > males;
            std::vector< std::pair<float, RE::Actor*> > females;
            auto result = scanner::scan_actors(2000.f, males, females);
            if (result && males.size()) {
                int n = scanner::nth_nearest(3, males);
                if (n > 0) {
                    RE::Actor* actor = males[0].second;
                    if (!skyrim_util::is_valid_for_event(actor))
                        return false;
                    //auto roll = ctx._rng.gen_float(0.f, 1.f);
                    auto roll = -1.f; // always trigger for testing
                    auto arousal = stats::stat_manager::get_actor_arousal(actor);
                    logger_custom::debug("forced_scene_1npc::trigger_phase1 roll={}, arousal={}, trigger => {}", roll, arousal, roll <= arousal);
                    if (roll > arousal) {
                        return false;
                    }
                    _npc1_handle = actor->GetHandle();
                    logger_custom::debug("forced_scene_1npc::trigger_phase1 found an actor");
                    return true;
                }
            }

            return false;
        }

#if 0
        std::optional<fsm::fsm_state> forced_scene_1npc::trigger_phase2(game::context& ctx) {
            auto aptr = _npc1_handle.get();
            if (!aptr) {
                logger_custom::error("forced_scene_1npc::trigger_phase2 called, but _npc1_handle is undefined");
                set_retry_cooldown(ctx);
                return std::nullopt;
            }
            auto actor = aptr.get();

            logger_custom::debug("forced_scene_1npc::trigger_phase2 initializing force greet");

            // Set global variables evaluated in dialog conditions
            form_cache::MiniMolestForceGreetType->value = 1.f;
            form_cache::MiniMolestForceGreetOutcome->value = 1.f;

            // Start force greet AI package
            actor_util::add_package_override(actor, form_cache::MiniMolestForceGreetPackage, 100, 1);
            actor->EvaluatePackage(true, false);

            return fsm::state_wait_dialog_finished{
                ctx._clock.get_play_second() + ctx._settings._force_greet_timeout_p, // force greet timeout
                // pre-save (identical to on_timeout)
                [this](game::context& ctx) -> std::optional<fsm::fsm_state> {
                    logger_custom::debug("state_sleep::trigger_phase2 timer tick callback pre-save abort force greet");
                    release_assert(form_cache::MiniMolestForceGreetType != nullptr);
                    release_assert(form_cache::MiniMolestForceGreetOutcome != nullptr);
                    form_cache::MiniMolestForceGreetType->value = 0.f;
                    form_cache::MiniMolestForceGreetOutcome->value = 0.f;
                    auto actorPtr = _npc1_handle.get();
                    if (actorPtr) {
                        auto actor = actorPtr.get();
                        actor_util::remove_package_override(actor, form_cache::MiniMolestForceGreetPackage);
                        actor->EvaluatePackage(true, false);
                    }
                    _npc1_handle = RE::ActorHandle();
                    set_retry_cooldown(ctx);
                    return fsm::state_idle{};
                },
                // on_timeout
                [this](game::context& ctx) -> std::optional<fsm::fsm_state> {
                    logger_custom::debug("state_sleep::trigger_phase2 timer tick callback force greet timed out");
                    release_assert(form_cache::MiniMolestForceGreetType != nullptr);
                    release_assert(form_cache::MiniMolestForceGreetOutcome != nullptr);
                    form_cache::MiniMolestForceGreetType->value = 0.f;
                    form_cache::MiniMolestForceGreetOutcome->value = 0.f;
                    auto actorPtr = _npc1_handle.get();
                    if (actorPtr) {
                        auto actor = actorPtr.get();
                        actor_util::remove_package_override(actor, form_cache::MiniMolestForceGreetPackage);
                        actor->EvaluatePackage(true, false);
                    }
                    _npc1_handle = RE::ActorHandle();
                    set_retry_cooldown(ctx);
                    return fsm::state_idle{};
                },
                // on_dialog_finished
                [this](game::context& ctx, RE::Actor* a_speaker) -> std::optional<fsm::fsm_state> {
                    // Code for 'on_dialog_finished' chain
                    logger_custom::debug("state_sleep::trigger_phase2 dialog finished callback force greet dialog done");

                    release_assert(form_cache::MiniMolestForceGreetType != nullptr);
                    release_assert(form_cache::MiniMolestForceGreetOutcome != nullptr);
                    auto outcome = form_cache::MiniMolestForceGreetOutcome->value;
                    form_cache::MiniMolestForceGreetType->value = 0.f;
                    form_cache::MiniMolestForceGreetOutcome->value = 0.f;
                    auto actorPtr = _npc1_handle.get();
                    if (!actorPtr || outcome != 1 || actorPtr.get() != a_speaker) {   // if speaker != npc1, something is wrong
                        if (actorPtr) {
                            auto actor = actorPtr.get();
                            actor_util::remove_package_override(actor, form_cache::MiniMolestForceGreetPackage);
                            actor->EvaluatePackage(true, false);
                            _npc1_handle = RE::ActorHandle();
                        }
                        set_retry_cooldown(ctx);
                        return fsm::state_idle{};
                    }

                    auto actor = actorPtr.get();
                    actor_util::remove_package_override(actor, form_cache::MiniMolestForceGreetPackage);

                    animation::backhug backhug{};
                    backhug.start(actor);     // start can only fail, if actor is null, so here, it can't
                    logger_custom::debug("after backhug.start: _hugger_handle = {}", backhug._hugger_handle.native_handle());

                    // Caution when chaining callbacks like here! We cannot reference stuff from the previous callback context, because it is gone, when the fsm state changes.
                    // So either pass per value, use std::move, or pass 'this'

                    // backhug sleep1
                    return fsm::state_sleep(
                        ctx._clock.get_play_second() + backhug._sleep1_duration_seconds,
                        // pre-save abort
                        [this, backhug = std::move(backhug)](game::context& a_context) mutable -> std::optional<fsm::fsm_state> {
                            backhug.stop();
                            set_retry_cooldown(a_context);
                            return fsm::state_idle{};
                        },
                        // sleep finished
                        [this, backhug = std::move(backhug)](game::context& a_context) mutable -> std::optional<fsm::fsm_state> {
                            logger_custom::debug("before backhug._sleep1: _hugger_handle = {}", backhug._hugger_handle.native_handle());
                            release_assert(backhug._sleep1); // can't be undefined, see above
                            backhug._sleep1();
                            logger_custom::debug("after backhug._sleep1: _hugger_handle = {}", backhug._hugger_handle.native_handle());
                            // backhug sleep2
                            return fsm::state_sleep(
                                a_context._clock.get_play_second() + backhug._sleep2_duration_seconds,
                                // pre-save abort
                                [this, backhug = std::move(backhug)](game::context& a_context) mutable -> std::optional<fsm::fsm_state> {
                                    backhug.stop();
                                    set_retry_cooldown(a_context);
                                    return fsm::state_idle{};
                                },
                                // sleep finished
                                [this, backhug = std::move(backhug)](game::context& a_context) mutable -> std::optional<fsm::fsm_state> {
                                    logger_custom::debug("before backhug._sleep2: _hugger_handle = {}", backhug._hugger_handle.native_handle());
                                    release_assert(backhug._sleep2); // can't be undefined, see above
                                    backhug._sleep2();
                                    logger_custom::debug("after backhug._sleep2: _hugger_handle = {}", backhug._hugger_handle.native_handle());
                                    // start struggle game
                                    a_context._struggle_game.start();
                                    // poll struggle game, until it ends
                                    return fsm::state_poll(
                                        // pre-save
                                        [this, backhug = std::move(backhug)](game::context& a_context) mutable -> std::optional<fsm::fsm_state> {
                                            a_context._struggle_game.reset();
                                            backhug.stop();
                                            set_retry_cooldown(a_context);
                                            return fsm::state_idle{};
                                        },
                                        // poll minigame end
                                        [this, backhug = std::move(backhug)](game::context& a_context) mutable -> std::optional<fsm::fsm_state> {
                                            auto struggle_state = a_context._struggle_game.get_state();
                                            if (struggle_state == minigame::struggle_game::state::playing)
                                                return std::nullopt;
                                            backhug.stop();
                                            release_assert(struggle_state != minigame::struggle_game::state::idle);  // something wrong, this shouldn't be possible
                                            if (struggle_state != minigame::struggle_game::state::lost) {
                                                // Successfully struggled free, TODO: update statistics
                                                set_regular_cooldown(a_context);
                                                return fsm::state_idle{};
                                            }

                                            // Struggle game lost, start scene
                                            std::vector<RE::Actor*> actors;
                                            std::vector<RE::Actor*> submissives;

                                            auto player = RE::PlayerCharacter::GetSingleton();
                                            release_assert(player != nullptr);
                                            actors.push_back(player);
                                            auto actorPtr = _npc1_handle.get();
                                            if (!actorPtr) {
                                                set_retry_cooldown(a_context);
                                                return fsm::state_idle{};
                                            }
                                            actors.push_back(actorPtr.get());
                                            submissives.push_back(player);

                                            auto hook_names = a_context._scene_starter.create_hook_names();
                                            auto handle = a_context._rng.gen_float(1.f, 10000000.f);

                                            SKSE::ModCallbackEvent modEvent;
                                            modEvent.eventName = RE::BSFixedString("MiniUtilStartScene");
                                            modEvent.strArg = hook_names._sl_hook_postfix;
                                            modEvent.numArg = handle;
                                            modEvent.sender = nullptr;

                                            auto inputSrc = SKSE::GetModCallbackEventSource();
                                            release_assert(inputSrc != nullptr);
                                            inputSrc->SendEvent(&modEvent);

                                            return fsm::state_serve_scene_parameters(
                                                a_context._clock.get_play_second() + a_context._settings._papyrus_sattelite_timeout_seconds,
                                                handle,
                                                std::move(actors),
                                                std::move(submissives),
                                                // pre-save
                                                [this](game::context& ctx) -> std::optional<fsm::fsm_state> {
                                                    logger_custom::debug("fsm::state_serve_scene_parameters pre-save abort");
                                                    set_retry_cooldown_xl(ctx, ctx._settings._scene_duration_max_seconds);
                                                    return fsm::state_idle{};
                                                },
                                                // error/timeout
                                                [this](game::context& ctx) -> std::optional<fsm::fsm_state> {
                                                    logger_custom::debug("fsm::state_serve_scene_parameters timed out");
                                                    set_retry_cooldown_xl(ctx, ctx._settings._scene_duration_max_seconds);
                                                    return fsm::state_idle{};
                                                },
                                                // success
                                                [this,handle = handle,hook_names = hook_names](game::context& ctx) -> std::optional<fsm::fsm_state> {
                                                    logger_custom::debug("fsm::state_serve_scene_parameters scene parameters served successfully");
                                                    return fsm::state_wait_scene_started(
                                                        ctx._clock.get_play_second() + ctx._settings._papyrus_sattelite_timeout_seconds,
                                                        handle,
                                                        // pre-save
                                                        [this](game::context& ctx) -> std::optional<fsm::fsm_state> {
                                                            logger_custom::debug("fsm::state_wait_scene_started pre-save abort");
                                                            set_retry_cooldown_xl(ctx, ctx._settings._scene_duration_max_seconds);
                                                            return fsm::state_idle{};
                                                        },
                                                        // error/timeout
                                                        [this](game::context& ctx) -> std::optional<fsm::fsm_state> {
                                                            logger_custom::debug("fsm::state_wait_scene_started timed out");
                                                            set_retry_cooldown_xl(ctx, ctx._settings._scene_duration_max_seconds);
                                                            return fsm::state_idle{};
                                                        },
                                                        // success
                                                        [this,hook_names = hook_names](game::context& ctx) -> std::optional<fsm::fsm_state> {
                                                            logger_custom::debug("fsm::state_wait_scene_started scene was started");
                                                            return fsm::state_wait_for_mod_event(
                                                                ctx._clock.get_play_second() + ctx._settings._scene_duration_max_seconds,
                                                                hook_names._sl_hook_end_name,
                                                                // pre-save
                                                                [this](game::context& ctx) -> std::optional<fsm::fsm_state> {
                                                                    logger_custom::debug("fsm::state_wait_scene_started pre-save abort");
                                                                    set_retry_cooldown_xl(ctx, ctx._settings._scene_duration_max_seconds);
                                                                    return fsm::state_idle{};
                                                                },
                                                                // timeout
                                                                [this](game::context& ctx) -> std::optional<fsm::fsm_state> {
                                                                    logger_custom::debug("fsm::state_wait_for_mod_event timed out");
                                                                    set_retry_cooldown(ctx);
                                                                    return fsm::state_idle{};
                                                                },
                                                                // success
                                                                [this](game::context& ctx) -> std::optional<fsm::fsm_state> {
                                                                    logger_custom::debug("fsm::state_wait_for_mod_event event received");
                                                                    // TODO: Update statistics
                                                                    set_regular_cooldown(ctx);
                                                                    return fsm::state_idle{};
                                                                }
                                                            );

                                                        }

                                                    );
                                                }
                                            );

                                        }
                                    );



                                }
                            );
                        }
                    );
                }
            };

        }
#endif


        std::optional<fsm::fsm_state> forced_scene_1npc::trigger_phase2(game::context& ctx) {
            auto aptr = _npc1_handle.get();
            if (!aptr) {
                logger_custom::error("forced_scene_1npc::trigger_phase2 called, but _npc1_handle is undefined");
                set_retry_cooldown(ctx);
                return std::nullopt;
            }
            auto actor = aptr.get();

            logger_custom::debug("forced_scene_1npc::trigger_phase2 initializing force greet");

            // Set global variables evaluated in dialog conditions
            form_cache::MiniMolestForceGreetType->value = 1.f;
            form_cache::MiniMolestForceGreetOutcome->value = 1.f;

            // Start force greet AI package
            actor_util::add_package_override(actor, form_cache::MiniMolestForceGreetPackage, 100, 1);
            actor->EvaluatePackage(true, false);

            _is_active = true;

            return fsm::state_wait_dialog_finished(
                ctx._clock.get_play_second() + ctx._settings._force_greet_timeout_p,
                std::make_pair(
                    forced_scene_1npc_ids::timeout,
                    [this](game::context& c) { return this->wait_dialog_finished_timeout_cb(c); }
                ),
                std::make_pair(
                    forced_scene_1npc_ids::dialog_finished,
                    [this](game::context& c, RE::Actor* a) { return this->wait_dialog_finished_finished_cb(c, a); }
                )    
            );
        }
    
        std::optional<fsm::fsm_state> forced_scene_1npc::wait_dialog_finished_timeout_cb(game::context& ctx) {
            logger_custom::debug("forced_scene_1npc::wait_dialog_finished_timeout_cb force greet timed out");
            release_assert(form_cache::MiniMolestForceGreetType != nullptr);
            release_assert(form_cache::MiniMolestForceGreetOutcome != nullptr);
            form_cache::MiniMolestForceGreetType->value = 0.f;
            form_cache::MiniMolestForceGreetOutcome->value = 0.f;
            auto actorPtr = _npc1_handle.get();
            if (actorPtr) {
                auto actor = actorPtr.get();
                actor_util::remove_package_override(actor, form_cache::MiniMolestForceGreetPackage);
                actor->EvaluatePackage(true, false);
            }
            _npc1_handle = RE::ActorHandle();
            set_retry_cooldown(ctx);
            return fsm::state_idle{};
        }
        
        std::optional<fsm::fsm_state> forced_scene_1npc::wait_dialog_finished_finished_cb(game::context& ctx, RE::Actor* a_speaker) {
            logger_custom::debug("forced_scene_1npc::wait_dialog_finished_timeout_cb force greet finished");
            release_assert(form_cache::MiniMolestForceGreetType != nullptr);
            release_assert(form_cache::MiniMolestForceGreetOutcome != nullptr);
            form_cache::MiniMolestForceGreetType->value = 0.f;
            form_cache::MiniMolestForceGreetOutcome->value = 0.f;
            auto actorPtr = _npc1_handle.get();
            if (actorPtr) {
                auto actor = actorPtr.get();
                actor_util::remove_package_override(actor, form_cache::MiniMolestForceGreetPackage);
                actor->EvaluatePackage(true, false);
            }
            _npc1_handle = RE::ActorHandle();
            set_retry_cooldown(ctx);
            return fsm::state_idle{};
        }

    }

}

/*
// Deine State-Struktur aus der Frage (leicht angepasst für Namensräume)
struct state_wait_dialog_finished {
    float _timeout_p = 0.f;
    std::pair<uint32_t, std::function<std::optional<fsm::fsm_state>(game::context&)>> on_timeout;
    std::pair<uint32_t, std::function<std::optional<fsm::fsm_state>(game::context&, RE::Actor*)>> on_dialog_finished;
};

// Test
void OnGameLoad() {
    // 1. Die langlebige Instanz deiner Kette wird geladen/erstellt
    event_chain::forced_scene_1npc::forced_scene_1npc* my_chain = new event_chain::forced_scene_1npc::forced_scene_1npc();

    // 2. Diese IDs hast du soeben aus dem Savegame gelesen:
    uint32_t loaded_timeout_id = event_chain::forced_scene_1npc::forced_scene_1npc_ids::timeout; // 1
    uint32_t loaded_dialog_id = event_chain::forced_scene_1npc::forced_scene_1npc_ids::dialog_finished; // 2

    // 3. Den State vollautomatisch über das Template rekonstruieren:
    state_wait_dialog_finished restored_state;
    restored_state._timeout_p = 30.0f;

    // Hier übergeben wir die Ziel-Signatur als Template-Argument
    restored_state.on_timeout = bind_callback<
        std::optional<fsm::fsm_state>(event_chain::forced_scene_1npc::forced_scene_1npc::*)(game::context&)
    >(loaded_timeout_id, my_chain);

    restored_state.on_dialog_finished = bind_callback<
        std::optional<fsm::fsm_state>(event_chain::forced_scene_1npc::forced_scene_1npc::*)(game::context&, RE::Actor*), // Signatur
        event_chain::forced_scene_1npc::forced_scene_1npc, // Kettentyp
        RE::Actor*                      // Zusätzliche Argumente für das Lambda
    >(loaded_dialog_id, my_chain);

    // TESTEN DES AUFRUFS:
    game::context *ctx = nullptr;
    RE::Actor* test_speaker = nullptr;

    // Das System führt die Logik aus der Kette sauber aus!
    if (restored_state.on_timeout.second) restored_state.on_timeout.second(*ctx);
    if (restored_state.on_dialog_finished.second) restored_state.on_dialog_finished.second(*ctx, test_speaker);

    delete my_chain;
}
*/