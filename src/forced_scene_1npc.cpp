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
#include "papyrus_utils.h"
#include "serialization.h"
#include "snippets.h"

namespace event_chain {

    bool delayed::serialize(SKSE::SerializationInterface* a_serde, float a_current_time) {
        uint8_t active = _is_active;
        auto next_play_second = _next_play_second - a_current_time;  // save offset to current time, because clock starts at 0 again, when game is loaded
        if (next_play_second < 0.f) next_play_second = 0.f;
        
        return a_serde->WriteRecordData(&next_play_second, sizeof(next_play_second))
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
        bool forced_scene_1npc::serialize(SKSE::SerializationInterface* a_serde, float a_current_time) {
            // delayed fields and active flag
            if (!delayed::serialize(a_serde, a_current_time)) return false;

            // backhug data
            uint8_t has_opt = _backhug_data.has_value();
            if (!a_serde->WriteRecordData(&has_opt, sizeof(has_opt))) return false;
            if (has_opt 
                && !serialization::SaveNiPoint3(a_serde, _backhug_data.value().markerTarget)
                && !serialization::SaveNiPoint3(a_serde, _backhug_data.value().markerRot)
                && !a_serde->WriteRecordData(&_backhug_data.value().AngleZ_raw, sizeof(_backhug_data.value().AngleZ_raw)))
                return false;

            // SL hook name
            has_opt = _sl_hook_data.has_value();
            if (!a_serde->WriteRecordData(&has_opt, sizeof(has_opt))) return false;
            if (has_opt
                && !serialization::SaveString(a_serde, _sl_hook_data.value()._sl_hook_postfix)
                && !serialization::SaveString(a_serde, _sl_hook_data.value()._sl_hook_end_name))
                return false;

            // Actor Form id
            if (!serialization::SaveActorHandle(a_serde, _npc1_handle)) return false;

            uint8_t set_away = _set_away;
            if (!a_serde->WriteRecordData(&set_away, sizeof(set_away))) return false;
            
            return true;    
        }

        int32_t forced_scene_1npc::deserialize(SKSE::SerializationInterface* a_serde) {
            int32_t read_bytes = 0;
            uint32_t current_read = 0;

            // base class
            read_bytes = delayed::deserialize(a_serde);
            if (read_bytes < 0) return -1;

            // backhug data
            uint8_t has_opt;
            _backhug_data.reset();  // shouldn't be necessary, because, when loading the game, the _events vector is created from scratch
            current_read = a_serde->ReadRecordData(&has_opt, sizeof(has_opt));
            if (current_read != sizeof(has_opt)) return -1; else read_bytes += current_read;
            if (has_opt) {
                animation::backhug::backhug_data data;
                current_read = serialization::LoadNiPoint3(a_serde, data.markerTarget);
                if (current_read < 0) return -1; else read_bytes += current_read; // current_read isn't necessarily = sizeof(NiPoint3)
                current_read = serialization::LoadNiPoint3(a_serde, data.markerRot);
                if (current_read < 0) return -1; else read_bytes += current_read;
                current_read = a_serde->ReadRecordData(&data.AngleZ_raw, sizeof(data.AngleZ_raw));
                if (current_read != sizeof(data.AngleZ_raw)) return -1; else read_bytes += current_read;
                _backhug_data = data;
            }

            // SL hook name
            _sl_hook_data.reset();
            current_read = a_serde->ReadRecordData(&has_opt, sizeof(has_opt));
            if (current_read != sizeof(has_opt)) return -1; else read_bytes += current_read;
            if (has_opt) {
                sl_scenes::sl_scene_hook_names data;
                current_read = serialization::LoadString(a_serde, data._sl_hook_postfix);
                if (current_read < 0) return -1; else read_bytes += current_read;
                current_read = serialization::LoadString(a_serde, data._sl_hook_end_name);
                if (current_read < 0) return -1; else read_bytes += current_read;
                _sl_hook_data = data;
            }

            current_read = serialization::LoadActorHandle(a_serde, _npc1_handle);
            if (current_read < 0) return -1; else read_bytes += current_read;
 
            uint8_t set_away;  // TODO remove, I don't use Frontier anymore
            current_read = a_serde->ReadRecordData(&set_away, sizeof(set_away));
            if (current_read != sizeof(set_away)) return -1; else read_bytes += current_read;
            _set_away = set_away;
            
            return read_bytes;
        }

        /////////////////////////////////////////////////////////////////////////////////////////////////////
        //
        // Really important, don't forget: When the event chain is started, and at all places, where it is ended 
        // (early returns!), the _is_active flag must be updated (otherwise the event manager doesn't find the 
        // active event chain.  
        // Also, don't forget to enable save games again, after disabling them !!!
        // 
        /////////////////////////////////////////////////////////////////////////////////////////////////////

        bool forced_scene_1npc::trigger_phase1(game::context& ctx) {
            logger_custom::debug("forced_scene_1npc::trigger_phase1");

            std::vector< std::pair<float, RE::Actor*> > males;
            std::vector< std::pair<float, RE::Actor*> > females;
            auto result = scanner::scan_actors(2000.f, males, females); // if more events doing this, better reuse results
            if (result && males.size()) {
                int n = scanner::nth_nearest(3, males);
                if (n > 0) {
                    RE::Actor* actor = males[0].second;
                    if (!skyrim_util::is_valid_for_event(actor))
                        return false;
                    if (ctx._clock.get_game_day() + _delay_game_days < stats::stat_manager::get_last_event_time(actor)) {
                        logger_custom::debug("forced_scene_1npc::trigger_phase1 actor on cooldown");
                        return false;
                    }

                    auto roll = ctx._rng.gen_float(0.f, 1.f);
                    auto arousal = stats::stat_manager::get_actor_arousal(actor);
                    auto prob = arousal * arousal / 4.f; // parabolic * correction factor check interval / desired mean_time

                    logger_custom::debug("forced_scene_1npc::trigger_phase1 roll={}, prob={}, trigger => {}", roll, prob, roll <= prob);
                    if (roll > prob && !ModTesting) {
                        return false;
                    }
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
                set_retry_cooldown(ctx);
                return std::nullopt;
            }
            auto actor = aptr.get();

            if (Frontier1) { // TODO, remove Frontier, dynamically generated navmeshes at runtime suck (full of holes!)
                _set_away = Frontier1->SetAway(actor->GetFormID(), true, "MiniUtil.dll");
                logger_custom::debug("forced_scene_1npc::trigger_phase2 SetAway result={}", _set_away);
            }

            logger_custom::debug("forced_scene_1npc::trigger_phase2 initializing force greet");

            // Set global variables evaluated in dialog conditions
            form_cache::MiniMolestForceGreetType->value = 1.f;
            form_cache::MiniMolestForceGreetOutcome->value = 1.f;

            // Start force greet AI package
            actor_util::add_package_override(actor, form_cache::MiniMolestForceGreetPackage, 100, 1);
            actor->EvaluatePackage(true, false);

            _is_active = true;              //////////////////////////////////// set as active event chain (needed for serialization)

            return fsm::state_dialog_open(
                ctx._clock.get_play_second() + ctx._settings._force_greet_timeout_p,
                actor->GetHandle(),
                std::make_pair(
                    forced_scene_1npc_ids::dialog_open_timeout_cb_id,
                    [this](game::context& c) { return this->dialog_open_timeout_cb(c); }
                ),
                std::make_pair(
                    forced_scene_1npc_ids::dialog_open_success_cb_id,
                    [this](game::context& c, RE::Actor* a) { return this->dialog_open_success_cb(c, a); }
                )
            );

        }

        void forced_scene_1npc::reset_npc1_handle() {
            if (_set_away && Frontier1) {
                _set_away = false;
                auto aptr = _npc1_handle.get();
                if (aptr) {
                    auto result = Frontier1->SetAway(aptr->GetFormID(), false, "MiniUtil.dll");
                    logger_custom::debug("forced_scene_1npc::reset_npc1_handle SetAway result={}", result);
                }
            }
            _npc1_handle = RE::ActorHandle();
        }

        std::optional<fsm::fsm_state> forced_scene_1npc::dialog_open_timeout_cb(game::context& ctx) {
            logger_custom::debug("forced_scene_1npc::wait_dialog_finished_timeout_cb force greet timed out");
            release_assert(form_cache::MiniMolestForceGreetType != nullptr);
            release_assert(form_cache::MiniMolestForceGreetOutcome != nullptr);
            form_cache::MiniMolestForceGreetType->value = 0.f;
            form_cache::MiniMolestForceGreetOutcome->value = 0.f;
            auto actorPtr = _npc1_handle.get();
            if (actorPtr) {
                auto* actor = actorPtr.get();
                actor_util::remove_package_override(actor, form_cache::MiniMolestForceGreetPackage);
                actor->EvaluatePackage(true, false);
            }
            reset_npc1_handle();
            set_retry_cooldown(ctx);
            return fsm::state_idle{};
        }

        std::optional<fsm::fsm_state> forced_scene_1npc::dialog_open_success_cb(game::context& ctx, RE::Actor* a_speaker) {
            logger_custom::debug("forced_scene_1npc::wait_dialog_opened_success_cb force greet dialogmenu opened");
            return fsm::state_dialog_close(
                std::make_pair(
                    forced_scene_1npc_ids::dialog_close_topicinfo_cb_id,
                    [this](game::context& c, RE::Actor* a, RE::FormID f) { return this->dialog_close_topicinfo_cb(c, a, f); }
                ),
                std::make_pair(
                    forced_scene_1npc_ids::dialog_close_menu_cb_id,
                    [this](game::context& c) { return this->dialog_close_menu_cb(c); }
                )
            );

        }


        std::optional<fsm::fsm_state> forced_scene_1npc::dialog_close_topicinfo_cb(game::context& ctx, RE::Actor* a_speaker, RE::FormID a_form_id) {
            // topicinfos not needed, everything defined via gloabal variables atm.
            logger_custom::debug("forced_scene_1npc::wait_dialog_closed_topicinfo_cb topicinfo={}", a_form_id);
            return std::nullopt;
        }
        
        std::optional<fsm::fsm_state> forced_scene_1npc::dialog_close_menu_cb(game::context& ctx) {
            logger_custom::debug("forced_scene_1npc::dialog_close_menu_cb force greet dialogmenu closed");
            release_assert(form_cache::MiniMolestForceGreetType != nullptr);
            release_assert(form_cache::MiniMolestForceGreetOutcome != nullptr);
            auto outcome = form_cache::MiniMolestForceGreetOutcome->value;
            form_cache::MiniMolestForceGreetType->value = 0.f;
            form_cache::MiniMolestForceGreetOutcome->value = 0.f;
            auto actorPtr = _npc1_handle.get();
            if (!actorPtr || outcome != 1) {
                if (actorPtr) {
                    auto actor = actorPtr.get();
                    actor_util::remove_package_override(actor, form_cache::MiniMolestForceGreetPackage);
                    actor->EvaluatePackage(true, false);
                    reset_npc1_handle();
                }
                set_retry_cooldown(ctx);
                return fsm::state_idle{};
            }

            auto actor = actorPtr.get();
            actor_util::remove_package_override(actor, form_cache::MiniMolestForceGreetPackage);

            _backhug_data = animation::backhug::backhug_start(actor);

            serialization::save_blocker::block();                ////////////////////////////// Disable save games, until struggle and scene are finished

            return fsm::state_sleep(
                ctx._clock.get_play_second() + 0.5f,
                std::make_pair(
                    forced_scene_1npc_ids::backhug_sleep1_cb_id,
                    [this](game::context& c) { return this->backhug_sleep1_cb(c); }
                ));
        }

        std::optional<fsm::fsm_state> forced_scene_1npc::backhug_sleep1_cb(game::context& ctx) {
            release_assert(_backhug_data.has_value());
            animation::backhug::backhug_after_500ms(_backhug_data.value().markerTarget, _backhug_data.value().markerRot);
            return fsm::state_sleep(
                ctx._clock.get_play_second() + 0.5f,
                std::make_pair(
                    forced_scene_1npc_ids::backhug_sleep2_cb_id,
                    [this](game::context& c) { return this->backhug_sleep2_cb(c); }
                ));
        }

        std::optional<fsm::fsm_state> forced_scene_1npc::backhug_sleep2_cb(game::context& ctx) {
            auto actorPtr = _npc1_handle.get();
            if (!actorPtr) {  // error
                animation::backhug::backhug_stop_player();  // cleanup
                _backhug_data.reset();
                set_retry_cooldown(ctx);
                serialization::save_blocker::unblock();             ////////////////////////////// Enable save games again
                return fsm::state_idle{};
            }
            release_assert(_backhug_data.has_value());
            animation::backhug::backhug_after_1000ms(actorPtr, _backhug_data.value().AngleZ_raw);
            _backhug_data.reset();
            ctx._struggle_game.start();
            return fsm::state_poll(
                std::make_pair(
                    forced_scene_1npc_ids::struggle_poll_cb_id,
                    [this](game::context& c) { return this->struggle_poll_cb(c); }
                ));
        }
 
        std::optional<fsm::fsm_state> forced_scene_1npc::struggle_poll_cb(game::context& ctx) {
            auto struggle_state = ctx._struggle_game.get_state();
            if (struggle_state == minigame::struggle_game::state::playing)
                return std::nullopt;
            ctx._struggle_game.reset();
            animation::backhug::backhug_stop_player();
            auto actorPtr = _npc1_handle.get();
            if (!actorPtr) {  // error, actor gone
                set_retry_cooldown(ctx);
                serialization::save_blocker::unblock();             ////////////////////////////// Enable save games again
                return fsm::state_idle{};
            }
            auto actor = actorPtr.get();
            release_assert(actor != nullptr);
            animation::backhug::backhug_stop_actor(actor);
            release_assert(struggle_state != minigame::struggle_game::state::idle);  // something wrong, this shouldn't be possible
            if (struggle_state != minigame::struggle_game::state::lost) {
                // Successfully struggled free, TODO: update statistics
                reset_npc1_handle();
                set_regular_cooldown(ctx);
                serialization::save_blocker::unblock();             ////////////////////////////// Enable save games again
                return fsm::state_idle{};
            }

            _sl_hook_data = ctx._scene_starter.create_hook_names();
            auto handle = (float)_sl_hook_data.value()._sl_hook_id; //ctx._rng.gen_float(1.f, 10000000.f);

            SKSE::ModCallbackEvent modEvent;
            modEvent.eventName = RE::BSFixedString("MiniUtilStartScene");
            modEvent.strArg = _sl_hook_data.value()._sl_hook_postfix;
            modEvent.numArg = handle;
            modEvent.sender = nullptr;

            auto inputSrc = SKSE::GetModCallbackEventSource();
            release_assert(inputSrc != nullptr);
            inputSrc->SendEvent(&modEvent);

            return fsm::state_scene_start(
                ctx._clock.get_play_second() + ctx._settings._papyrus_sattelite_timeout_seconds,
                handle,
                std::make_pair(
                    forced_scene_1npc_ids::scene_start_timeout_cb_id, 
                    [this](game::context& c) { return this->scene_start_timeout_cb(c); }
                ),    
                std::make_pair(
                    forced_scene_1npc_ids::scene_start_success_cb_id,
                    [this](game::context& c, RE::TESQuest* a_this, float a_handle) { return this->scene_start_success_cb(c, a_this, a_handle); }
                )
            );
        }

        std::optional<fsm::fsm_state>  forced_scene_1npc::scene_error_return(game::context& ctx) {
            reset_npc1_handle();
            _sl_hook_data.reset();
            set_retry_cooldown(ctx);
            serialization::save_blocker::unblock();             ////////////////////////////// Enable save games again
            return fsm::state_idle{};
        }

        std::optional<fsm::fsm_state>  forced_scene_1npc::scene_error_return_xl(game::context& ctx) {
            reset_npc1_handle();
            _sl_hook_data.reset();
            set_retry_cooldown_xl(ctx, ctx._settings._scene_duration_max_seconds);
            serialization::save_blocker::unblock();             ////////////////////////////// Enable save games again
            return fsm::state_idle{};
        }

        std::optional<fsm::fsm_state> forced_scene_1npc::scene_start_timeout_cb(game::context& ctx) {
            logger_custom::debug("fsm::state_scene_start timed out");
            return scene_error_return(ctx);
        }

        std::pair<bool, std::optional<fsm::fsm_state>> forced_scene_1npc::scene_start_success_cb(game::context& ctx, RE::TESQuest* a_this, float a_handle) {
            if (!_sl_hook_data.has_value() || _sl_hook_data.value()._sl_hook_id != a_handle)  // invalid handle    
                return std::make_pair(false, scene_error_return(ctx));
            
            auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
            release_assert(vm != nullptr);
            auto policy = vm->GetObjectHandlePolicy();
            RE::VMHandle vm_handle = policy->GetHandleForObject(a_this->GetFormType(), a_this);
            if (vm_handle == policy->EmptyHandle()) {
                logger_custom::debug("state_scene_start error vm handle={}", vm_handle);
                return std::make_pair(false, scene_error_return(ctx));
            }
            RE::BSTSmartPointer<RE::BSScript::Object> scriptObject;
            if (!vm->FindBoundObject(vm_handle, "MiniMolestMain_Script", scriptObject) || !scriptObject) {
                logger_custom::debug("state_scene_start error scriptObject=", (void*)scriptObject.get());
                return std::make_pair(false, scene_error_return(ctx));
            }
            logger_custom::debug("state_scene_start Found script {}", "MiniMolestMain_Script");

            std::vector<RE::Actor*> actors;
            std::vector<RE::Actor*> submissives;

            auto player = RE::PlayerCharacter::GetSingleton();
            release_assert(player != nullptr);
            actors.push_back(player);
            auto actorPtr = _npc1_handle.get();
            if (!actorPtr) {
                return std::make_pair(false, scene_error_return(ctx));
            }
            actors.push_back(actorPtr.get());
            submissives.push_back(player);

            int success_count = 0;

            RE::BSTSmartPointer<RE::BSScript::Array> papyrus_actors;
            if (papyrus_utils::actor_array_to_papyrus(vm, policy, actors, papyrus_actors)) {
                logger_custom::debug("state_scene_start Array actors converted to Papyrus");
                auto variableIter = scriptObject->GetVariable("SceneActors");
                if (variableIter) {
                    success_count++;
                    logger_custom::debug("Setting SceneActors size = {}", actors.size());
                    RE::BSScript::Variable var;
                    var.SetArray(papyrus_actors);
                    *variableIter = var;
                }
            }

            RE::BSTSmartPointer<RE::BSScript::Array> papyrus_submissive_actors;
            if (papyrus_utils::actor_array_to_papyrus(vm, policy, submissives, papyrus_submissive_actors)) {
                logger_custom::debug("Array submissive_actors converted to Papyrus");
                auto variableIter = scriptObject->GetVariable("SceneSubmissiveActors");
                if (variableIter) {
                    success_count++;
                    logger_custom::debug("Setting SceneSubmissiveActors submissive_actors size = {}", submissives.size());
                    RE::BSScript::Variable var;
                    var.SetArray(papyrus_submissive_actors);
                    *variableIter = var;
                }
            }

            if (success_count != 2)
                return std::make_pair(false, scene_error_return(ctx));

            // Give the player a Frostfall Exposure buff, does no harm
            ReducePlayerExposure(-50.f);

            release_assert(_sl_hook_data.has_value());
            auto hook_name = _sl_hook_data.value()._sl_hook_end_name;
            _sl_hook_data.reset();

            return std::make_pair(true, fsm::state_scene_end(
                ctx._clock.get_play_second() + ctx._settings._scene_duration_max_seconds,
                a_handle,
                hook_name,
                std::make_pair(
                    forced_scene_1npc_ids::scene_end_timeout_cb_id,
                    [this](game::context& c) { return this->scene_end_timeout_cb(c); }
                ),
                std::make_pair(
                    forced_scene_1npc_ids::scene_end_papyrus_cb_id,
                    [this](game::context& c, float a_handle, int32_t a_thread_id) { return this->scene_end_papyrus_cb(c, a_handle, a_thread_id); }
                ),
                std::make_pair(
                    forced_scene_1npc_ids::scene_end_modevent_cb_id,
                    [this](game::context& c) { return this->scene_end_modevent_cb(c); }
                )));
        }

        std::optional<fsm::fsm_state> forced_scene_1npc::scene_end_timeout_cb(game::context& ctx) {
            logger_custom::debug("fsm::state_wait_scene_set_thread_id timed out");
            return scene_error_return_xl(ctx);
        }

        std::pair<bool, std::optional<fsm::fsm_state>> forced_scene_1npc::scene_end_papyrus_cb(game::context& ctx, float a_handle, int32_t a_thread_id) {
            logger_custom::debug("fsm::state_wait_scene_set_thread_id thread id = {}", a_thread_id);
			if (!_sl_hook_data.has_value() || _sl_hook_data.value()._sl_hook_id != a_handle || a_thread_id < 0)  // invalid handle or thread id    
                return std::make_pair(false, scene_error_return_xl(ctx));
            return std::make_pair(true, std::nullopt);
        }
        
        std::optional<fsm::fsm_state> forced_scene_1npc::scene_end_modevent_cb(game::context& ctx) {
            logger_custom::debug("fsm::state_scene_end event received");

            // TODO: More statistics, better probability calculations
            auto actorPtr = _npc1_handle.get();
            if (actorPtr) {
                stats::stat_manager::set_last_event_time(actorPtr.get(), ctx._clock.get_game_day());
            }
            reset_npc1_handle();
            set_regular_cooldown(ctx);
            serialization::save_blocker::unblock();             ////////////////////////////// Enable save games again
            return fsm::state_idle{};
        }

    }

}
