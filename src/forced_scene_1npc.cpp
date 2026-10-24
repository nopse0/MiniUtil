#include "forced_scene_1npc.h"
#include "game_context.h"
#include "form_cache.h"
#include "actor_util.h"
#include "state_wait_timeout.h"
#include "state_idle.h"
#include "state_serve_scene_parameters.h"
#include "fsm.h"
#include "scanner.h"
#include "backhug.h"
#include "skyrim_util.h"

namespace event_chain {

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
                _npc1_handle = actor->GetHandle();
                logger_custom::debug("forced_scene_1npc::trigger_phase1 found an actor");
                return true;
            }
        }

        return false;
	}

    std::optional<fsm::fsm_state> forced_scene_1npc::trigger_phase2(game::context& ctx) {
        auto aptr = _npc1_handle.get();
        if (!aptr) {
            logger_custom::error("forced_scene_1npc::trigger_phase2 called, but _npc1_handle is undefined");
            update_cooldown(ctx, false);
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
                update_cooldown(ctx, false);
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
                    update_cooldown(ctx, false);
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
                    // on_timeout
                    [this, backhug = std::move(backhug)](game::context& a_context) mutable -> std::optional<fsm::fsm_state> {
                        logger_custom::debug("before backhug._sleep1: _hugger_handle = {}", backhug._hugger_handle.native_handle());
                        release_assert(backhug._sleep1); // can't be undefined, see above
                        backhug._sleep1();
                        logger_custom::debug("after backhug._sleep1: _hugger_handle = {}", backhug._hugger_handle.native_handle());
                        // backhug sleep2
                        return fsm::state_sleep(
                            a_context._clock.get_play_second() + backhug._sleep2_duration_seconds,
                            [this, backhug = std::move(backhug)](game::context& a_context) mutable -> std::optional<fsm::fsm_state> {
                                logger_custom::debug("before backhug._sleep2: _hugger_handle = {}", backhug._hugger_handle.native_handle());
                                release_assert(backhug._sleep2); // can't be undefined, see above
                                backhug._sleep2();
                                logger_custom::debug("after backhug._sleep2: _hugger_handle = {}", backhug._hugger_handle.native_handle());
                                // start struggle game
                                a_context._struggle_game.start();
                                // poll struggle game, until it ends
                                return fsm::state_poll(
                                    [this, backhug = std::move(backhug)](game::context& a_context) mutable -> std::optional<fsm::fsm_state> {
                                        auto struggle_state = a_context._struggle_game.get_state();
                                        if (struggle_state == minigame::struggle_game::state::playing)
                                            return std::nullopt;
                                        backhug.stop();
                                        release_assert(struggle_state != minigame::struggle_game::state::idle);  // something wrong, this shouldn't be possible
                                        if (struggle_state != minigame::struggle_game::state::lost) {
                                            // Successfully struggled free, TODO: update statistics
                                            update_cooldown(a_context, true);
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
                                            update_cooldown(a_context, false);
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
                                            // error/timeout
                                            [this](game::context& ctx) -> std::optional<fsm::fsm_state> {
                                                logger_custom::debug("fsm::state_serve_scene_parameters timed out");
                                                update_cooldown(ctx, false);
                                                return fsm::state_idle{};
                                            },
                                            // success
                                            [this,handle=handle,hook_names=hook_names](game::context& ctx) -> std::optional<fsm::fsm_state> {
                                                logger_custom::debug("fsm::state_serve_scene_parameters scene parameters served successfully");
                                                return fsm::state_wait_scene_started(
                                                    ctx._clock.get_play_second() + ctx._settings._papyrus_sattelite_timeout_seconds,
                                                    handle,
                                                    // error/timeout
                                                    [this](game::context& ctx) -> std::optional<fsm::fsm_state> {
                                                        logger_custom::debug("fsm::state_wait_scene_started timed out");
                                                        update_cooldown(ctx, false);
                                                        return fsm::state_idle{};
                                                    },
                                                    [this,hook_names=hook_names](game::context& ctx) -> std::optional<fsm::fsm_state> {
                                                        logger_custom::debug("fsm::state_wait_scene_started scene was started");
                                                        return fsm::state_wait_for_mod_event(
                                                            ctx._clock.get_play_second() + ctx._settings._scene_duration_max_seconds,
                                                            hook_names._sl_hook_end_name,
                                                            [this](game::context& ctx) -> std::optional<fsm::fsm_state> {
                                                                logger_custom::debug("fsm::state_wait_for_mod_event timed out");
                                                                update_cooldown(ctx, false);
                                                                return fsm::state_idle{};
                                                            },
                                                            [this](game::context& ctx) -> std::optional<fsm::fsm_state> {
                                                                logger_custom::debug("fsm::state_wait_for_mod_event event received");
                                                                // TODO: Update statistics
                                                                update_cooldown(ctx, true);
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
}
