#include "forced_scene_1npc.h"
#include "game_context.h"
#include "form_cache.h"
#include "actor_util.h"
#include "state_force_greet_1npc.h"
#include "state_idle.h"
#include "fsm.h"

namespace event_chain {

	bool forced_scene_1npc::pre_trigger(game::context& ctx) {
        logger_custom::debug("");
        if (_next_game_day > ctx._clock.get_game_day()) {
            _trigger = false;
            return _trigger;
        }
        _trigger = true;
        return _trigger;
	}

    void forced_scene_1npc::update_cooldown(game::context& ctx) {
        logger_custom::debug("");
        if (!_trigger) {
            _next_play_second = ctx._clock.get_play_second() + _delay_retry_play_seconds;
        }
        else {
            _trigger = false;
            _next_play_second = ctx._clock.get_play_second() + _delay_play_seconds;
            _next_game_day = ctx._clock.get_game_day() + _delay_game_days;
        }
    }

    std::optional<fsm::fsm_state> forced_scene_1npc::trigger(fsm::fsm& fsm, game::context& ctx) {
        logger_custom::debug("");
        if (!_trigger)
            return std::nullopt;
        auto aptr = _force_greet_actor_handle.get();
        if (!aptr) {
            logger_custom::error("trigger() called, but _force_greet_actor_handle is undefined");
            _trigger = false;
            update_cooldown(ctx);
            return std::nullopt;
        }
        auto actor = aptr.get();

        // Set global variables evaluated in dialog conditions
        form_cache::MiniMolestForceGreetType->value = 1.f;
        form_cache::MiniMolestForceGreetOutcome->value = 1.f;

        // Start force greet AI package
        actor_util::add_package_override(actor, form_cache::MiniMolestForceGreetPackage, 100, 1);
        actor->EvaluatePackage(true, false);

        // Force greet timeout
        auto timeout_p = ctx._clock.get_play_second() + ctx._settings._force_greet_timeout_p;
        return fsm::state_force_greet_1npc{
            timeout_p,
            [this](fsm::fsm& a_fsm, game::context& ctx) -> std::optional<fsm::fsm_state> {
                // Code for 'on_timeout' chain
                form_cache::MiniMolestForceGreetType->value = 0.f;
                form_cache::MiniMolestForceGreetOutcome->value = 0.f;
                auto actorPtr = _force_greet_actor_handle.get();
                if (actorPtr) {
                    auto actor = actorPtr.get();
                    actor_util::remove_package_override(actor, form_cache::MiniMolestForceGreetPackage);
                    actor->EvaluatePackage(true, false);
                }
                _trigger = false;
                _force_greet_actor_handle = RE::ActorHandle();
                update_cooldown(ctx);
                return fsm::state_idle{};
            },
            [this](fsm::fsm& a_fsm, game::context& ctx, RE::Actor* a_speaker) -> std::optional<fsm::fsm_state> {
                // Code for 'on_dialog_finished' chain
                // TODO, atm. same as in on_timeout
                form_cache::MiniMolestForceGreetType->value = 0.f;
                form_cache::MiniMolestForceGreetOutcome->value = 0.f;
                auto actorPtr = _force_greet_actor_handle.get();
                if (actorPtr) {
                    auto actor = actorPtr.get();
                    actor_util::remove_package_override(actor, form_cache::MiniMolestForceGreetPackage);
                    actor->EvaluatePackage(true, false);
                }
                _trigger = false;
                _force_greet_actor_handle = RE::ActorHandle();
                update_cooldown(ctx);
                return fsm::state_idle{};
            }
        };
        
	}
}
