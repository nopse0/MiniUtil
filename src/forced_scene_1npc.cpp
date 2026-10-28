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

namespace event_chain {

    bool delayed::serialize(SKSE::SerializationInterface* a_serde, float a_current_time) {
        uint8_t active = _is_active;
        auto next_play_second = _next_play_second - a_current_time;  // save offset to current time, because clock starts at 0 again, when game is loaded
        if (next_play_second < 0.f) next_play_second = 0.f;
        return a_serde->WriteRecordData(&next_play_second, sizeof(next_play_second))
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
        bool forced_scene_1npc::serialize(SKSE::SerializationInterface* a_serde, float a_current_time) {
            if (!delayed::serialize(a_serde, a_current_time))
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
                    forced_scene_1npc_ids::wait_dialog_finished_timeout_id,
                    [this](game::context& c) { return this->wait_dialog_finished_timeout_cb(c); }
                ),
                std::make_pair(
                    forced_scene_1npc_ids::wait_dialog_finished_success_id,
                    [this](game::context& c, RE::Actor* a) { return this->wait_dialog_finished_success_cb(c, a); }
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

        std::optional<fsm::fsm_state> forced_scene_1npc::wait_dialog_finished_success_cb(game::context& ctx, RE::Actor* a_speaker) {
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

            _backhug_data = animation::backhug::backhug_start(actor);

            return fsm::state_sleep(
                ctx._clock.get_play_second() + 0.5f,
                std::make_pair(
                    forced_scene_1npc_ids::backhug_sleep1_id,
                    [this](game::context& c) { return this->backhug_sleep1_cb(c); }
                ));
        }


        std::optional<fsm::fsm_state> forced_scene_1npc::backhug_sleep1_cb(game::context& ctx) {
            release_assert(_backhug_data.has_value());
            animation::backhug::backhug_after_500ms(_backhug_data.value().markerTarget, _backhug_data.value().markerRot);
            return fsm::state_sleep(
                ctx._clock.get_play_second() + 0.5f,
                std::make_pair(
                    forced_scene_1npc_ids::backhug_sleep2_id,
                    [this](game::context& c) { return this->backhug_sleep2_cb(c); }
                ));
        }

        std::optional<fsm::fsm_state> forced_scene_1npc::backhug_sleep2_cb(game::context& ctx) {
            auto actorPtr = _npc1_handle.get();
            if (!actorPtr) {  // error
                animation::backhug::backhug_stop_player();  // cleanup
                _backhug_data.reset();
                set_retry_cooldown(ctx);
                return fsm::state_idle{};
            }
            release_assert(_backhug_data.has_value());
            animation::backhug::backhug_after_1000ms(actorPtr, _backhug_data.value().AngleZ_raw);
            _backhug_data.reset();
            ctx._struggle_game.start();
            return fsm::state_poll(
                std::make_pair(
                    forced_scene_1npc_ids::poll_struggle_game_id,
                    [this](game::context& c) { return this->poll_struggle_game_cb(c); }
                ));
        }
 
        std::optional<fsm::fsm_state> forced_scene_1npc::poll_struggle_game_cb(game::context& ctx) {
            auto struggle_state = ctx._struggle_game.get_state();
            if (struggle_state == minigame::struggle_game::state::playing)
                return std::nullopt;
            ctx._struggle_game.reset();
            animation::backhug::backhug_stop_player();
            auto actorPtr = _npc1_handle.get();
            if (!actorPtr) {  // error, actor gone
                set_retry_cooldown(ctx);
                return fsm::state_idle{};
            }
            auto actor = actorPtr.get();
            release_assert(actor != nullptr);
            animation::backhug::backhug_stop_actor(actor);
            release_assert(struggle_state != minigame::struggle_game::state::idle);  // something wrong, this shouldn't be possible
            if (struggle_state != minigame::struggle_game::state::lost) {
                // Successfully struggled free, TODO: update statistics
                _npc1_handle = RE::ActorHandle();
                set_regular_cooldown(ctx);
                return fsm::state_idle{};
            }

            _sl_hook_data = ctx._scene_starter.create_hook_names();
            auto handle = ctx._rng.gen_float(1.f, 10000000.f);

            SKSE::ModCallbackEvent modEvent;
            modEvent.eventName = RE::BSFixedString("MiniUtilStartScene");
            modEvent.strArg = _sl_hook_data.value()._sl_hook_postfix;
            modEvent.numArg = handle;
            modEvent.sender = nullptr;

            auto inputSrc = SKSE::GetModCallbackEventSource();
            release_assert(inputSrc != nullptr);
            inputSrc->SendEvent(&modEvent);

            return fsm::state_serve_scene_parameters(
                ctx._clock.get_play_second() + ctx._settings._papyrus_sattelite_timeout_seconds,
                handle,
                std::make_pair(
                    forced_scene_1npc_ids::serve_scene_parameters_timeout_id, 
                    [this](game::context& c) { return this->serve_scene_parameters_timeout_cb(c); }
                ),    
                std::make_pair(
                    forced_scene_1npc_ids::serve_scene_parameters_serve_id,
                    [this](game::context& c, RE::TESQuest* a_this, float a_handle) { return this->serve_scene_parameters_serve_cb(c, a_this, a_handle); }
                )
            );
        }

        std::optional<fsm::fsm_state>  forced_scene_1npc::scene_error_return(game::context& ctx) {
            _npc1_handle = RE::ActorHandle();
            _sl_hook_data.reset();
            set_retry_cooldown(ctx);
            return fsm::state_idle{};
        }

        std::optional<fsm::fsm_state>  forced_scene_1npc::scene_error_return_xl(game::context& ctx) {
            _npc1_handle = RE::ActorHandle();
            _sl_hook_data.reset();
            set_retry_cooldown_xl(ctx, ctx._settings._scene_duration_max_seconds);
            return fsm::state_idle{};
        }

        std::optional<fsm::fsm_state> forced_scene_1npc::serve_scene_parameters_timeout_cb(game::context& ctx) {
            logger_custom::debug("fsm::state_serve_scene_parameters timed out");
            return scene_error_return(ctx);
        }

        std::pair<bool, std::optional<fsm::fsm_state>> forced_scene_1npc::serve_scene_parameters_serve_cb(game::context& ctx, RE::TESQuest* a_this, float a_handle) {
            auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
            release_assert(vm != nullptr);
            auto policy = vm->GetObjectHandlePolicy();
            RE::VMHandle vm_handle = policy->GetHandleForObject(a_this->GetFormType(), a_this);
            if (vm_handle == policy->EmptyHandle()) {
                logger_custom::debug("state_serve_scene_parameters error vm handle={}", vm_handle);
                return std::make_pair(false, scene_error_return(ctx));
            }
            RE::BSTSmartPointer<RE::BSScript::Object> scriptObject;
            if (!vm->FindBoundObject(vm_handle, "MiniMolestMain_Script", scriptObject) || !scriptObject) {
                logger_custom::debug("state_serve_scene_parameters error scriptObject=", (void*)scriptObject.get());
                return std::make_pair(false, scene_error_return(ctx));
            }
            logger_custom::debug("state_serve_scene_parameters Found script {}", "MiniMolestMain_Script");

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
                logger_custom::debug("state_serve_scene_parameters Array actors converted to Papyrus");
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

            return std::make_pair(true, fsm::state_wait_scene_set_thread_id(
                ctx._clock.get_play_second() + ctx._settings._papyrus_sattelite_timeout_seconds,
                a_handle,
                std::make_pair(
                    forced_scene_1npc_ids::set_scene_thread_id_timeout_id,
                    [this](game::context& c) { return this->set_scene_thread_id_timeout_cb(c); }
                ),
                std::make_pair(
                    forced_scene_1npc_ids::set_scene_thread_id_set_id,
                    [this](game::context& c, float a_handle, int32_t a_thread_id) { return this->set_scene_thread_id_set_cb(c, a_handle, a_thread_id); }
                )
            ));
        }

        std::optional<fsm::fsm_state> forced_scene_1npc::set_scene_thread_id_timeout_cb(game::context& ctx) {
            logger_custom::debug("fsm::state_set_scene_parameters timed out");
            return scene_error_return_xl(ctx);
        }

        std::pair<bool, std::optional<fsm::fsm_state>> forced_scene_1npc::set_scene_thread_id_set_cb(game::context& ctx, float a_handle, int32_t a_thread_id) {
            logger_custom::debug("fsm::state_set_scene_parameters timed out");
            
            release_assert(_sl_hook_data.has_value());
            auto hook_name = _sl_hook_data.value()._sl_hook_end_name;
            _sl_hook_data.reset();

            return std::make_pair(false, fsm::state_wait_for_mod_event(
                ctx._clock.get_play_second() + ctx._settings._scene_duration_max_seconds,
                hook_name,
                std::make_pair(
                    forced_scene_1npc_ids::wait_dialog_finished_timeout_id,
                    [this](game::context& c) { return this->wait_for_mod_event_timeout_cb(c); }
                ),
                std::make_pair(
                    forced_scene_1npc_ids::wait_dialog_finished_success_id,
                    [this](game::context& c) { return this->wait_for_mod_event_received_cb(c); }
                )
            ));
        }

        std::optional<fsm::fsm_state> forced_scene_1npc::wait_for_mod_event_timeout_cb(game::context& ctx) {
            _npc1_handle = RE::ActorHandle();
            set_retry_cooldown_xl(ctx, ctx._settings._scene_duration_max_seconds);
            return fsm::state_idle{};
        }
        
        std::optional<fsm::fsm_state> forced_scene_1npc::wait_for_mod_event_received_cb(game::context& ctx) {
            logger_custom::debug("fsm::state_wait_for_mod_event event received");
            // TODO: Update statistics
            _npc1_handle = RE::ActorHandle();
            set_regular_cooldown(ctx);
            return fsm::state_idle{};
        }

    }

}
