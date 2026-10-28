#include "state_all.h"
#include "fsm.h"
#include "controller.h"
#include "struggle_game.h"
#include "scanner.h"
#include "actor_util.h"
#include "form_cache.h"
#include "sl_scenes.h"
#include "random.h"
#include "papyrus_utils.h"
#include "skyrim_util.h"
#include "serialization.h"

#if 0
namespace controller {

    // Eigene Definition, da die original-Methode private ist
    static void MoveTo_Helper(
        RE::TESObjectREFR* a_this,
        const RE::ObjectRefHandle& a_targetHandle,
        RE::TESObjectCELL* a_targetCell,
        RE::TESWorldSpace* a_selfWorldSpace,
        const RE::NiPoint3& a_position,
        const RE::NiPoint3& a_rotation)
    {
        if (!a_this) return;

        // Wir definieren den Funktions-Typen genau wie in der CommonLib
        using func_t = void(RE::TESObjectREFR*, const RE::ObjectRefHandle&, RE::TESObjectCELL*, RE::TESWorldSpace*, const RE::NiPoint3&, const RE::NiPoint3&);

        // Nutzung der plattformunabhängigen IDs von CommonLibSSE-NG
        static REL::Relocation<func_t> func{ RELOCATION_ID(56227, 56626) };

        func(a_this, a_targetHandle, a_targetCell, a_selfWorldSpace, a_position, a_rotation);
    }

    static inline bool is_playing()
    {
        auto ui = RE::UI::GetSingleton();
        if (!ui) {
            return false;
        }

        // 1. Block if the game is natively paused (Esc menu, inventory, etc.)
        if (ui->GameIsPaused()) {
            return false;
        }

        // 2. Block if an "Application Menu" is open. 
        // This internal engine flag covers the Main Menu (title screen) and Loading screens.
        if (ui->IsApplicationMenuOpen()) {
            return false;
        }

        // 3. SkyUI / MCM Specific Protection (Crucial for Skyrim Souls RE compatibility)
        // The journal menu includes the MCM. Since it might be unpaused by mods, 
        // we explicitly check its presence on the stack.
        if (ui->IsMenuOpen("Journal Menu")) {
            return false;
        }

        // 4. (Optional) Prevent running if the console or full-screen menus are open
        if (ui->IsMenuOpen("Console") || ui->IsMenuOpen("Loading Menu")) {
            return false;
        }

        return true;
    }

    static inline float get_game_day()
    {
        auto calendar = RE::Calendar::GetSingleton();
        if (!calendar) return 0.f;
        return calendar->GetDaysPassed();
    }

    // Tries to restore the pristine state directly after construction (before 'new game'/'load game')
    void controller::revert() {
        logger::debug("resetting");
        if (_state == state_t::Struggle) {
            logger::warn("Resetting controller while in Struggle state. This may leave the game in an inconsistent state.");
            minigame::struggle_game::get_instance().reset();
            _scheduledTasks.clear();
            if (_back_hug_actor_handle) {
                logger::debug("reset: stopping back-hug");
                auto actorPtr = _back_hug_actor_handle.get();   // RE::Actor::LookupByHandle(_back_hug_actor_ref_handle);
                if (!actorPtr) {
                    logger::warn("stop back hug: actor lookup failed");
                } else {
                    stop_back_hug(actorPtr.get());
                }
                _back_hug_actor_handle.reset();   // = RE::RefHandle(0);
            }
        }
        else if (_state == state_t::InScene) {
            sl_scenes::sl_scene::get_instance().revert();
        }

        goto_idle_state();

        // Factory defaults
        _next_event_min_game_day = 0.f;
        _next_event_min_play_second = 0.f;
        _play_second = 0.f;
    }

    // Cleans temporary variables and sets state to Idle 
    void controller::goto_idle_state() 
    {
        // Default state
        _state = state_t::Idle;
        _busy = false;
        _force_greet_start_play_second = 0.f;
        _force_greet_actor_handle.reset();

        _scene_start_play_second = 0.f;
        _back_hug_actor_handle.reset();
        _sl_hook_id = 0;  // because of the delays between events, there should be no collisions with older hook calls, when loading a previous game, I think
        _scheduledTasks.clear();
    }

    void controller::serialize(SKSE::SerializationInterface* a_serde)
    {
        a_serde->WriteRecordData(&_play_second, sizeof(_play_second));
        a_serde->WriteRecordData(&_next_event_min_game_day, sizeof(_next_event_min_game_day));
        a_serde->WriteRecordData(&_next_event_min_play_second, sizeof(_next_event_min_play_second));

        // Remark: The InScene state is persisted, but when loading the game, it will immediately timeout (because we don't persist the scene start time), and Idle state
        // will be entered (with the min failed event delay for the next event)
        if (_state != state_t::Idle && _state != state_t::ForceGreet)
            logger::error("controller::save: we shouldn't be in state {} here, this won't work!", static_cast<uint8_t>(_state));
        a_serde->WriteRecordData(&_state, sizeof(_state));
        uint8_t bool_byte = static_cast<uint8_t>(_busy);
        a_serde->WriteRecordData(&bool_byte, sizeof(bool_byte));
        a_serde->WriteRecordData(&_force_greet_start_play_second, sizeof(_force_greet_start_play_second));

        RE::FormID formId = 0;
        auto actorPtr = _force_greet_actor_handle.get();
        if (actorPtr) {
            formId = actorPtr->GetFormID();
        }
        a_serde->WriteRecordData(&formId, sizeof(formId));
    }

    uint32_t controller::deserialize(SKSE::SerializationInterface* a_serde)
    {
        uint32_t read = 0;
        read += a_serde->ReadRecordData(&_play_second, sizeof(_play_second));
        read += a_serde->ReadRecordData(&_next_event_min_game_day, sizeof(_next_event_min_game_day));
        read += a_serde->ReadRecordData(&_next_event_min_play_second, sizeof(_next_event_min_play_second));
    
        read += a_serde->ReadRecordData(&_state, sizeof(_state));
        uint8_t bool_byte;
        read += a_serde->ReadRecordData(&bool_byte, sizeof(bool_byte));
        _busy = bool_byte;
        read += a_serde->ReadRecordData(&_force_greet_start_play_second, sizeof(_force_greet_start_play_second));
  
        RE::FormID savedFormID = 0;     // Lot of boiler-plate code for serializing actor handles
        read += a_serde->ReadRecordData(&savedFormID, sizeof(savedFormID));
        if (savedFormID != 0) {
            RE::FormID resolvedFormID = 0;
            // CRITICAL: SKSE korrigiert hier die FormID basierend auf der aktuellen Load-Order!
            if (a_serde->ResolveFormID(savedFormID, resolvedFormID)) {

                // Hole das Objekt über die korrigierte FormID
                auto refr = RE::TESForm::LookupByID<RE::Actor>(resolvedFormID);
                if (refr) {
                    // Erstelle wieder dein sicheres ObjectRefHandle für deine Klasse
                    _force_greet_actor_handle = refr->GetHandle();   // savedFormID == 0 is handled in 'revert', don't have to do this here
                }
            }
        }
        else {   // but safe is safe :)
            _force_greet_actor_handle.reset();
        }
        return read;
    }

    void controller::update_play_second(float a_play_time_delta_seconds) {
        _play_second += a_play_time_delta_seconds;
    }

    void controller::set_event_success_min_delays()
    {
        logger::debug("controller::set_event_success_min_delays");
        _next_event_min_game_day = get_game_day() + _event_min_success_delay_game_days;
        _next_event_min_play_second = get_play_second() + _event_min_success_delay_play_seconds;
    }

    void controller::set_event_failure_min_delays()
    {
        logger::debug("controller::set_event_failure_min_delays");
        _next_event_min_play_second = get_play_second() + _event_min_failure_delay_play_seconds;
    }

    // Scheduling helpers
    void controller::schedule_task_after_play_seconds(float a_delay, std::function<void()> a_fn)
    {
        schedule_task_at_play_second(get_play_second() + a_delay, std::move(a_fn));
    }

    void controller::schedule_task_at_play_second(float a_execute_play_second, std::function<void()> a_fn)
    {
        ScheduledTask t;
        t.execute_play_second = a_execute_play_second;
        t.fn = std::move(a_fn);
        _scheduledTasks.emplace_back(std::move(t));
    }

    std::string controller::next_sl_hook()
    {
        if (_sl_hook_id < 255)
            _sl_hook_id++;
        else
            _sl_hook_id = 1;
        auto postfix = std::format("MiniMolest{:02X}", _sl_hook_id);
        _sl_hook_name = "AnimationEnd_" + postfix;
        return postfix;
    }

    RE::BSEventNotifyControl controller::ProcessEvent(const SKSE::ModCallbackEvent* a_event, RE::BSTEventSource<SKSE::ModCallbackEvent>* a_eventSource)
    {
        if (a_event && _sl_hook_id > 0) {
            logger::trace("ProcessEvent: eventName = {}", a_event->eventName.c_str());

            if (a_event->eventName == _sl_hook_name) {
                logger::debug("controller::ProcessEvent: AnimationEnd event received: {}", _sl_hook_name.c_str());
                goto_idle_state();
                set_event_success_min_delays();
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    }
    
    void controller::on_timer_tick(float a_play_time_delta_seconds)
    {
        logger::trace("Controller received timer tick, time delta = {}", a_play_time_delta_seconds);

        // advance time first (so scheduling uses same time base as Papyrus Wait)
        update_play_second(a_play_time_delta_seconds);

        // run scheduled tasks that are due
        for (auto it = _scheduledTasks.begin(); it != _scheduledTasks.end();) {
            if (it->execute_play_second <= get_play_second()) {
                auto fn = std::move(it->fn);
                it = _scheduledTasks.erase(it);
                try {
                    if (fn) fn();
                }
                catch (const std::exception& e) {
                    logger::error("Scheduled task threw exception: {}", e.what());
                }
                catch (...) {
                    logger::error("Scheduled task threw unknown exception");
                }
            }
            else {
                ++it;
            }
        }

        // no actions, until unfinished tasks are completed
        if (_scheduledTasks.size() > 0) {
            logger::trace("on_timer_tick aborted, scheduled tasks pending: {}", _scheduledTasks.size());
            return;
        }

        if (!is_playing()) {
            logger::trace("on_timer_tick aborted, not playing");
            return;
        }

        // on the fly initialization
        if (_next_event_min_play_second == 0.f)
            set_event_success_min_delays();

        if (get_play_second() < _next_event_min_play_second || get_game_day() < _next_event_min_game_day) {
            logger::trace(
                "on_timer_tick aborted, events on cooldown, "
                "get_play_second:{} < _next_event_min_play_second::{} || get_game_day:{} < _next_event_min_game_day:{}",
                get_play_second(), _next_event_min_play_second, get_game_day(), _next_event_min_game_day);
            return;
        }

        if (!_busy) {
            std::vector< std::pair<float, RE::Actor*> > males;
            std::vector< std::pair<float, RE::Actor*> > females;
            auto result = scanner::scan_actors(2000.f, males, females);
            if (result && males.size()) {
                int n = scanner::nth_nearest(3, males);
                if (n > 0) {
                    /*_busy = true;
                    _state = state_t::ForceGreet;
                    _force_greet_start_play_second = get_play_second();
                    form_cache::MiniMolestForceGreetType->value = 1.f;
                    form_cache::MiniMolestForceGreetOutcome->value = 1.f;
                    RE::Actor* actor = males[0].second;
                    _npc1_handle = actor->GetHandle();
                    actor_util::add_package_override(actor, form_cache::MiniMolestForceGreetPackage, 100, 1);
                    logger::info("Started ForceGreet package on actor {}", (void*)actor);
                    actor->EvaluatePackage(true, false);*/

                    if (n >= 2) {
                        auto quest = papyrus_utils::lookup<RE::TESQuest>("MiniMolestQuest");
                        if (!quest) {
                            logger::error("Unable to find MiniMolestQuest");
                        }
                        else {
                            RE::Actor* forceGreeter1 = males[0].second;
                            RE::Actor* forceGreeter2 = males[1].second;
                            // Aliases: 0 player, 1 ForceGreeter1, 2 ForceGreeter2, 3 ForceGreeter3
                            bool result = papyrus_utils::force_to_alias(quest, 1, forceGreeter1) && papyrus_utils::force_to_alias(quest, 2, forceGreeter2);
                            if (!result) {
                                logger::error("Setting reference aliases failed");
                            }
                            else {
                                _state = state_t::ForceGreet2;
                                _force_greet_start_play_second = get_play_second();
                                form_cache::MiniMolestForceGreetType->value = 1.f;
                                form_cache::MiniMolestForceGreetOutcome->value = 1.f;
                                _force_greet_actor_handle = forceGreeter1->GetHandle();
                                _force_greet_actor_second_handle = forceGreeter2->GetHandle();
                                actor_util::add_package_override(forceGreeter1, form_cache::MiniMolestForceGreetWithOneFollower, 100, 1);
                                actor_util::add_package_override(forceGreeter2, form_cache::MiniMolestForceGreetFollowerPackage, 100, 1);
                                forceGreeter2->EvaluatePackage(true, false);
                                forceGreeter1->EvaluatePackage(true, false);
                                logger::info("Started ForceGreet with two npcs");
                            }
                        }
                    }
                }
            }
        }
        else {
            if (_state == state_t::ForceGreet) {
                if (get_play_second() - _force_greet_start_play_second > _force_greet_timeout_play_seconds) {
                    logger::debug("ForceGreet timeout reached, removing package override and resetting state");
                    auto actorPtr = _force_greet_actor_handle.get();
                    if (actorPtr) {
                        auto actor = actorPtr.get();
                        actor_util::remove_package_override(actor, form_cache::MiniMolestForceGreetPackage);
                        actor->EvaluatePackage(true, false);
                    }
                    goto_idle_state();
                    set_event_failure_min_delays();
                }
            }
            else if (_state == state_t::Struggle) {
                auto stat = minigame::struggle_game::get_instance().get_state();
                if (stat != minigame::struggle_game::state::playing) {
                    logger::debug("Struggle minigame finished");
                    //_busy = false;
                    //_state = state_t::Idle;
                    //set_event_success_min_delays();
                    if (_back_hug_actor_handle) {
                        logger::debug("Struggle minigame finished, stopping back-hug");
                        auto actorPtr = _back_hug_actor_handle.get();
                        if (!actorPtr) {
                            logger::warn("stop back hug: actor lookup failed");
                            return;
                        }
                        //_back_hug_actor_ref_handle.reset();
                        stop_back_hug(actorPtr.get());

                        if (stat == minigame::struggle_game::state::lost) {
							//_busy = true;
							_state = state_t::InScene;
							std::vector<RE::Actor*> actors;
                            std::vector<RE::Actor*> submissives;
                            auto player = RE::PlayerCharacter::GetSingleton();
                            actors.push_back(player);
							actors.push_back(actorPtr.get());
                            submissives.push_back(player);
                            auto hookPostfix = next_sl_hook();
                            _scene_start_play_second = get_play_second();
							sl_scenes::sl_scene::get_instance().trigger_scene(actors, submissives, hookPostfix);
                        }
                        else {  // minigame won
                            goto_idle_state();
                        }
                    }
                    else {
                        logger::warn("Struggle minigame finished, but no back-hug actor handle stored");
                        goto_idle_state();
                    }
                }
            }
            else if (_state == state_t::InScene) {
                if (sl_scenes::sl_scene::get_instance().get_error() || get_play_second() - _scene_start_play_second > _scene_timeout_play_seconds) {
                    logger::debug("Scene failed or timed out, resetting state");
                    sl_scenes::sl_scene::get_instance().revert();
                    goto_idle_state();
                    set_event_failure_min_delays();
                }
            }

        }

    }

    void controller::on_dialog_end(RE::Actor* a_speaker) {
        if (_state != state_t::ForceGreet || _force_greet_actor_handle != a_speaker->GetHandle()) {
            logger::warn("on_dialog_end called but state is not ForceGreet or actor does not match, ignoring");
            return;
        }

        auto outcome = form_cache::MiniMolestForceGreetOutcome->value;
        form_cache::MiniMolestForceGreetType->value = 0.f;
        form_cache::MiniMolestForceGreetOutcome->value = 0.f;

        if (outcome == 1.f) {
            logger::info("Dialogue ended with actor {}, starting struggle minigame", (void*)a_speaker);
            _state = state_t::Struggle;
            _back_hug_actor_handle = start_back_hug(a_speaker);
            minigame::struggle_game::get_instance().start(_struggle_timeout_seconds);
        }
    }

    RE::ActorHandle controller::start_back_hug(RE::Actor* a_actor) {
        if (!a_actor) {
            logger::warn("start_back_hug: a_actor is null");
            return RE::ActorHandle();
        }

        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            logger::warn("start_back_hug: player singleton missing");
            return RE::ActorHandle();
        }

        // 1) Package override + force package evaluation
        actor_util::add_package_override(a_actor, form_cache::MiniMolestDoNothingPackage, 100, 1);
        a_actor->EvaluatePackage(true, false);

        // 2) Restrain the NPC (life state) and block movement
        a_actor->SetLifeState(RE::ACTOR_LIFE_STATE::kRestrained);
        a_actor->GetActorRuntimeData().boolFlags.set(RE::Actor::BOOL_FLAGS::kMovementBlocked);
        a_actor->StopMoving(0.0f);

        // 3) Make player AI-driven by disabling player controls
		player->SetAIDriven(true);
        //player->SetPlayerControls(false);

        // Ensure third person if currently in first person
        if (auto cam = RE::PlayerCamera::GetSingleton(); cam && cam->IsInFirstPerson()) {
            cam->ForceThirdPerson();
        }

        // Papyrus-like simple flow
        const float pi = 3.14159265358979323846f;
        // Papyrus uses these exact offsets
        int Xaxis = 0;
        int Yaxis = -50;
        // Read angle once and use it as-is (do NOT convert)
        float AngleZ_raw = player->GetAngleZ();

        // Compute Papyrus-style offsets using AngleZ exactly as Papyrus does
        float rMoveX = (std::sin(AngleZ_raw) * static_cast<float>(Yaxis)) + (std::cos(AngleZ_raw) * static_cast<float>(Xaxis));
        float rMoveY = (std::cos(AngleZ_raw) * static_cast<float>(Yaxis)) - (std::sin(AngleZ_raw) * static_cast<float>(Xaxis));

        RE::NiPoint3 markerRot = form_cache::MiniMolestAnimMarker ? form_cache::MiniMolestAnimMarker->data.angle : RE::NiPoint3{};
        markerRot.z = 0.0f;

        RE::NiPoint3 markerTarget = player->GetPosition();
        markerTarget.x += rMoveX;
        markerTarget.y += rMoveY;

        // store handles for lambdas
        RE::ObjectRefHandle playerHandle = player->GetHandle();
        //RE::RefHandle playerRefHandle = playerObjHandle.native_handle();
        RE::ActorHandle actorHandle = a_actor->GetHandle();
        //RE::RefHandle actorRefHandle = actorHandle.native_handle();

        logger::info("start_back_hug: playerAngleRaw={} rMoveX={:.2f} rMoveY={:.2f}", AngleZ_raw, rMoveX, rMoveY);

        // Move marker after 0.5s (papyrus: Utility.Wait 0.5; MiniMolestAnimMarker.MoveTo)
        schedule_task_after_play_seconds(0.5f, [markerTarget, markerRot, playerHandle]() {
            //auto playerLocal = RE::TESObjectREFR::LookupByHandle(playerRefHandle);
            auto playerLocal = playerHandle.get();
            if (!playerLocal) {
                logger::warn("scheduled: player lookup failed");
                return;
            }
            if (!form_cache::MiniMolestAnimMarker) {
                logger::warn("scheduled: MiniMolestAnimMarker missing");
                return;
            }

            MoveTo_Helper(
                form_cache::MiniMolestAnimMarker,
                playerHandle,
                playerLocal->GetParentCell(),
                playerLocal->GetWorldspace(),
                markerTarget,
                markerRot);

            auto markerPos = form_cache::MiniMolestAnimMarker->GetPosition();
            logger::info("[scheduled] marker moved. markerPos={:.2f},{:.2f},{:.2f}", markerPos.x, markerPos.y, markerPos.z);
            });

        // Wait another 0.5s then Move actor to marker and set angle (papyrus: akactor.MoveTo(MiniMolestAnimMarker); akactor.setangle(...))
        schedule_task_after_play_seconds(1.0f, [actorHandle, AngleZ_raw]() {
            //auto actorPtr = RE::Actor::LookupByHandle(actorRefHandle);
            auto actorPtr = actorHandle.get();
            if (!actorPtr) {
                logger::warn("scheduled: actor lookup failed");
                return;
            }

            if (form_cache::MiniMolestAnimMarker) {
                actorPtr->MoveTo(form_cache::MiniMolestAnimMarker);
            }
            else {
                logger::warn("scheduled: MiniMolestAnimMarker not cached for actor move");
            }

            // Papyrus sets angle = AngleZ + angle (angle == 0 in original). We reuse AngleZ_raw as-is.
            actorPtr->SetAngle(RE::NiPoint3{ 0.0f, 0.0f, AngleZ_raw });

            // fire animations
            actorPtr->NotifyAnimationGraph("BaboBackHugStartM");
            if (auto playerLocal = RE::PlayerCharacter::GetSingleton()) {
                playerLocal->NotifyAnimationGraph("BaboBackHugStartF");
            }
            });

        logger::info("start_back_hug: scheduled back hug sequence for actor {}", (void*)a_actor);
        return actorHandle;
    }

    void controller::stop_back_hug(RE::Actor* a_actor) {
        if (!a_actor) {
            logger::warn("stop_back_hug: null actor");
            return;
        }

        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            logger::warn("end_back_hug: player singleton missing");
            return;
        }

        // Optional: Falls eure SDK/Headers SetVehicle anbieten, könnt ihr das aktivieren:
        // player->SetVehicle(nullptr);
        // a_actor->SetVehicle(nullptr);

        // Entferne das DoNothing-Package-Override, falls gesetzt
        actor_util::remove_package_override(a_actor, form_cache::MiniMolestDoNothingPackage);

        // Setze LifeState und Movement-Flag zurück
        a_actor->SetLifeState(RE::ACTOR_LIFE_STATE::kAlive);
        a_actor->GetActorRuntimeData().boolFlags.reset(RE::Actor::BOOL_FLAGS::kMovementBlocked);
        a_actor->StopMoving(0.0f);

        // Re-aktiviere Player Controls und AIDriven-Status zurücksetzen
        player->SetAIDriven(false);
        //player->SetPlayerControls(true);
        // Falls ihr Game.SetPlayerAIDriven(true) gesetzt habt, müsst ihr ggf. den Game-/Player-AI-Status zurücksetzen.
        // (In start_back_hug haben wir nur SetPlayerControls(false) verwendet.)

        // Gib dem Actor wieder seine AI-Pakete
        a_actor->EvaluatePackage(true, false);

        // Ziehe beide Skeletons aus der gepaarten Pose
        a_actor->NotifyAnimationGraph("IdleForceDefaultState");
        if (auto playerLocal = RE::PlayerCharacter::GetSingleton()) {
            playerLocal->NotifyAnimationGraph("IdleForceDefaultState");
        }

        logger::info("stop_back_hug: restored actor {} and re-enabled player controls", (void*)a_actor);
    }


    uint32_t manager::next_timer_tick(float a_play_time_delta_seconds)
    {
        update_play_second(a_play_time_delta_seconds);   // advance the clock

        if (run_scheduled_tasks())
            return 1;    // scheduled tasks not done yet, don't interfer
    
        //fsm::context_t ctx{ *this, _event_selector };
        //_fsm.handle_event(fsm::event_timer_tick{ctx});
    
        return 6;
    }



}

#endif

namespace controller {
    controller::controller() : _context{_settings, _clock, _event_manager, minigame::struggle_game::get_instance(), _scene_starter, random::random::get_instance()} {
        _event_manager.initialize(_context);
    }


    // SKSE Serialization
    void controller::revert() {
        _context._clock.set_play_second(0.f);
    }
    bool controller::serialize(SKSE::SerializationInterface* a_serde) {
        logger::info("controller::serialize: saving event_manager");
        if (!_event_manager.serialize(a_serde, _context))
            return false;

        logger::info("controller::serialize: saving fsm state");
        bool success = serialization::SaveState(a_serde, _fsm.get_state());
        
        return success;
    }

    int32_t controller::deserialize(SKSE::SerializationInterface* a_serde) {
        int32_t read_bytes = 0;
        int32_t current_read = 0;
        logger_custom::debug("controller::deserialize");

        current_read = _event_manager.deserialize(a_serde);
        if (current_read < 0) return -1;
        read_bytes += current_read;

        auto ev = _event_manager.get_active_event();
        logger_custom::debug("controller::deserialize read_bytes={} ev={}", read_bytes, (void*)ev);

        fsm::fsm_state state;

        if (ev == nullptr) {
            current_read = serialization::LoadFreeState(a_serde, state);
            if (current_read < 0) return -1; else read_bytes += current_read;
        }
        else {
            // 3. Über std::visit die ECHTE Kette im Vektor als Referenz auspacken
            // Das Sternchen (*ev) dereferenziert den Zeiger für visit
            current_read = std::visit([a_serde, &state](auto& concrete_chain) -> int32_t {
                return serialization::LoadEventState(&concrete_chain, a_serde, state);
                }, *ev);
            if (current_read < 0) return -1; else read_bytes += current_read;
        }
        _fsm.set_state(std::move(state));

        logger_custom::debug("controller::deserialize current_read={} read_bytes={}", current_read, read_bytes);
        return read_bytes;
    }

    // Mod event callback
    RE::BSEventNotifyControl controller::ProcessEvent(const SKSE::ModCallbackEvent* a_event, RE::BSTEventSource<SKSE::ModCallbackEvent>* a_eventSource) {
        if (a_event) {
            logger::trace("controller::ProcessEvent: eventName = {}", a_event->eventName.c_str());
            _fsm.process_mod_event(_context, a_event->eventName);
        }
        return RE::BSEventNotifyControl::kContinue;
    }

    // Functions called from Papyrus scripts
    void controller::on_dialog_end(RE::Actor* a_speaker) {
        _fsm.process_dialog_finished(_context, a_speaker);
    }

    bool controller::on_get_scene_parameters(RE::TESQuest* a_this, float a_handle) {
        return _fsm.process_get_scene_parameters(_context, a_this, a_handle);
    }

    bool controller::on_set_scene_thread_id(float a_handle, int32_t a_thread_id) {
        return _fsm.process_set_scene_thread_id(_context, a_handle, a_thread_id);
    }

    // Skyrim Update loop hook
    uint32_t controller::next_timer_tick(float a_play_time_delta_seconds) {

        // only advance clock, if playing
        if (!skyrim_util::is_playing()) {
            logger_custom::trace("controller::next_timer_tick aborted, not playing");
            return 6;
        }

        _clock.update_play_second(a_play_time_delta_seconds);
        logger_custom::trace("controller::next_timer_tick current play time = {}", _clock.get_play_second());

        _fsm.process_timer_tick(_context);
        return 6;  // next tick in 6 frames
    }

}
