#include "minigame.h"
#include "scanner.h"
#include "actor_util.h"
#include "form_cache.h"
#include "controller.h"
#include "sl_scenes.h"
#include "random.h"

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

    void controller::reset() {
        logger::debug("resetting");
        if (_state == state_t::Struggle) {
            logger::warn("Resetting controller while in Struggle state. This may leave the game in an inconsistent state.");
            minigame::struggle_game::get_instance().reset();
            _scheduledTasks.clear();
            if (_back_hug_actor_ref_handle) {
                logger::debug("reset: stopping back-hug");
                auto actorPtr = _back_hug_actor_ref_handle.get();   // RE::Actor::LookupByHandle(_back_hug_actor_ref_handle);
                if (!actorPtr) {
                    logger::warn("stop back hug: actor lookup failed");
                    return;
                }
                _back_hug_actor_ref_handle.reset();   // = RE::RefHandle(0);
                stop_back_hug(actorPtr.get());
            }
        }
        // state_t::InScene:   nothing to do
     
        // Default state
        _state = state_t::Idle;
        _busy = false;
        _next_event_min_game_day = 0.f;
        _next_event_min_play_second = 0.f;
        _force_greet_start_play_second = 0.f;
        _force_greet_actor_handle.reset(); //= nullptr;
        _back_hug_actor_ref_handle.reset();      //RE::RefHandle(0);
        _scheduledTasks.clear();

    }

    void save(SKSE::SerializationInterface* a_serde)
    {

    }


    void controller::update_play_second(float a_play_time_delta_seconds) {
        _play_second += a_play_time_delta_seconds;
    }

    void controller::set_event_success_min_delays()
    {
        _next_event_min_game_day = get_game_day() + _event_min_success_delay_game_days;
        _next_event_min_play_second = get_play_second() + _event_min_success_delay_play_seconds;
    }

    void controller::set_event_failure_min_delays()
    {
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

    RE::BSEventNotifyControl controller::ProcessEvent(const SKSE::ModCallbackEvent* a_event, RE::BSTEventSource<SKSE::ModCallbackEvent>* a_eventSource)
    {
        logger::debug(" controller::ProcessEvent a_event = {}", (void*)a_event);
        if (a_event) {
            std::string eventName = a_event->eventName.c_str();
            logger::debug("ProcessEvent: eventName = {}", eventName);


        }
        return  RE::BSEventNotifyControl::kContinue;
    }
    
    void controller::on_timer_tick(float a_play_time_delta_seconds)
    {
        logger::trace("Controller received timer tick");

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
                    _busy = true;
                    _state = state_t::ForceGreet;
                    _force_greet_start_play_second = get_play_second();
                    form_cache::MiniMolestForceGreetType->value = 1.f;
                    form_cache::MiniMolestForceGreetOutcome->value = 1.f;
                    RE::Actor* actor = males[0].second;
                    _force_greet_actor_handle = actor->GetHandle();    //.native_handle();
                    actor_util::add_package_override(actor, form_cache::MiniMolestForceGreetPackage, 100, 1);
                    logger::info("Started ForceGreet package on actor {}", (void*)actor);
                    actor->EvaluatePackage(true, false);
                }
            }
        }
        else {
            if (_state == state_t::ForceGreet) {
                if (get_play_second() - _force_greet_start_play_second > _force_greet_timeout_play_seconds) {
                    logger::debug("ForceGreet timeout reached, removing package override and resetting state");
                    _busy = false;
                    _state = state_t::Idle;
                    auto actorPtr = _force_greet_actor_handle.get();  //RE::Actor::LookupByHandle(_force_greet_actor_handle);
                    if (actorPtr) {
                        auto actor = actorPtr.get();
                        actor_util::remove_package_override(actor, form_cache::MiniMolestForceGreetPackage);
                        actor->EvaluatePackage(true, false);
                    }
                    _force_greet_actor_handle.reset();  // = RE::RefHandle(0);
                    set_event_failure_min_delays();
                }
            }
            else if (_state == state_t::Struggle) {
                auto stat = minigame::struggle_game::get_instance().get_state();
                if (stat != minigame::struggle_game::state::playing) {
                    logger::debug("Struggle minigame finished, resetting state");
                    _busy = false;
                    _state = state_t::Idle;
                    set_event_success_min_delays();
                    if (_back_hug_actor_ref_handle) {
                        logger::debug("Struggle minigame finished, stopping back-hug");
                        auto actorPtr = _back_hug_actor_ref_handle.get();  // RE::Actor::LookupByHandle(_back_hug_actor_ref_handle);
                        if (!actorPtr) {
                            logger::warn("stop back hug: actor lookup failed");
                            return;
                        }
                        _back_hug_actor_ref_handle.reset();   // = RE::RefHandle(0);
                        stop_back_hug(actorPtr.get());

                        if (stat == minigame::struggle_game::state::lost) {
							_busy = true;
							_state = state_t::InScene;
							std::vector<RE::Actor*> actors;
                            std::vector<RE::Actor*> submissives;
                            auto player = RE::PlayerCharacter::GetSingleton();
                            actors.push_back(player);
							actors.push_back(actorPtr.get());
                            submissives.push_back(player);
							sl_scenes::sl_scene::get_instance().trigger_scene(actors, submissives);
                        }

                    }
                    else {
                        logger::warn("Struggle minigame finished, but no back-hug actor handle stored");
                    }
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
            _back_hug_actor_ref_handle = start_back_hug(a_speaker);
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
}
