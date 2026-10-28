#include "actor_util.h"
#include "form_cache.h"
#include "state_all.h"
#include "fsm.h"
#include "skyrim_util.h"
#include "backhug.h"

namespace animation {

    namespace backhug {
        backhug_data backhug_start(RE::Actor* a_actor) {
            release_assert(a_actor != nullptr);

            auto player = RE::PlayerCharacter::GetSingleton();
            release_assert(player != nullptr);

            actor_util::add_package_override(a_actor, form_cache::MiniMolestDoNothingPackage, 100, 1);
            a_actor->EvaluatePackage(true, false);

            a_actor->SetLifeState(RE::ACTOR_LIFE_STATE::kRestrained);
            a_actor->GetActorRuntimeData().boolFlags.set(RE::Actor::BOOL_FLAGS::kMovementBlocked);
            a_actor->StopMoving(0.0f);

            player->SetAIDriven(true);

            if (auto cam = RE::PlayerCamera::GetSingleton(); cam && cam->IsInFirstPerson()) {
                cam->ForceThirdPerson();
            }

            //const float pi = 3.14159265358979323846f;
            int Xaxis = 0;
            int Yaxis = -50;
            float AngleZ_raw = player->GetAngleZ();

            float rMoveX = (std::sin(AngleZ_raw) * static_cast<float>(Yaxis)) + (std::cos(AngleZ_raw) * static_cast<float>(Xaxis));
            float rMoveY = (std::cos(AngleZ_raw) * static_cast<float>(Yaxis)) - (std::sin(AngleZ_raw) * static_cast<float>(Xaxis));

            RE::NiPoint3 markerRot = form_cache::MiniMolestAnimMarker ? form_cache::MiniMolestAnimMarker->data.angle : RE::NiPoint3{};
            markerRot.z = 0.0f;

            RE::NiPoint3 markerTarget = player->GetPosition();
            markerTarget.x += rMoveX;
            markerTarget.y += rMoveY;

            logger_custom::debug("back_hug::start playerAngleRaw={} rMoveX={:.2f} rMoveY={:.2f}", AngleZ_raw, rMoveX, rMoveY);

            return backhug_data{ markerTarget, markerRot, AngleZ_raw };
        }

        void backhug_after_500ms(RE::NiPoint3 markerTarget, RE::NiPoint3 markerRot) {
            auto playerLocal = RE::PlayerCharacter::GetSingleton();
            release_assert(playerLocal != nullptr);
            release_assert(form_cache::MiniMolestAnimMarker != nullptr);  // this is not "noexcept", but since this also terminates the game, doesn't matter
            skyrim_util::move_to(
                form_cache::MiniMolestAnimMarker,
                playerLocal->GetHandle(),
                playerLocal->GetParentCell(),
                playerLocal->GetWorldspace(),
                markerTarget,
                markerRot);
            //auto markerPos = form_cache::MiniMolestAnimMarker->GetPosition();
            //logger_custom::debug("[scheduled] marker moved. markerPos={:.2f},{:.2f},{:.2f}", markerPos.x, markerPos.y, markerPos.z);
        }

        void backhug_after_1000ms(RE::ActorPtr actorPtr, float AngleZ_raw) {
            release_assert(actorPtr);
            release_assert(form_cache::MiniMolestAnimMarker != nullptr);
            actorPtr->MoveTo(form_cache::MiniMolestAnimMarker);
            actorPtr->SetAngle(RE::NiPoint3{ 0.0f, 0.0f, AngleZ_raw });
            actorPtr->NotifyAnimationGraph("BaboBackHugStartM");
            auto playerLocal = RE::PlayerCharacter::GetSingleton();
            release_assert(playerLocal != nullptr);
            playerLocal->NotifyAnimationGraph("BaboBackHugStartF");
        }

        void backhug_stop_actor(RE::Actor* a_actor) {
            release_assert(a_actor != nullptr);
            actor_util::remove_package_override(a_actor, form_cache::MiniMolestDoNothingPackage);
            a_actor->SetLifeState(RE::ACTOR_LIFE_STATE::kAlive);
            a_actor->GetActorRuntimeData().boolFlags.reset(RE::Actor::BOOL_FLAGS::kMovementBlocked);
            a_actor->StopMoving(0.0f);
            a_actor->EvaluatePackage(true, false);
            a_actor->NotifyAnimationGraph("IdleForceDefaultState");
        }

        void backhug_stop_player() {
            auto player = RE::PlayerCharacter::GetSingleton();
            release_assert(player != nullptr);
            player->SetAIDriven(false);
            player->NotifyAnimationGraph("IdleForceDefaultState");
        }




        /*bool backhug::start(RE::Actor* a_actor) {
            if (!a_actor) {
                logger::warn("backhug::start: a_actor is null");
                return false;
            }

            auto player = RE::PlayerCharacter::GetSingleton();
            release_assert(player != nullptr);

            _hugger_handle = a_actor->GetHandle();

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

            logger_custom::debug("start_back_hug: playerAngleRaw={} rMoveX={:.2f} rMoveY={:.2f}", AngleZ_raw, rMoveX, rMoveY);

            _sleep1 = [markerTarget, markerRot]() {
                auto playerLocal = RE::PlayerCharacter::GetSingleton();
                release_assert(playerLocal != nullptr);
                release_assert(form_cache::MiniMolestAnimMarker != nullptr);  // this is not "noexcept", but since this also terminates the game, doesn't matter

                skyrim_util::move_to(
                    form_cache::MiniMolestAnimMarker,
                    playerLocal->GetHandle(),
                    playerLocal->GetParentCell(),
                    playerLocal->GetWorldspace(),
                    markerTarget,
                    markerRot);

                auto markerPos = form_cache::MiniMolestAnimMarker->GetPosition();
                logger_custom::debug("[scheduled] marker moved. markerPos={:.2f},{:.2f},{:.2f}", markerPos.x, markerPos.y, markerPos.z);
                };

            _sleep2 = [hugger_handle = _hugger_handle, AngleZ_raw]() {
                logger_custom::debug("in backhug._sleep2: _hugger_handle = {}", hugger_handle.native_handle());
                auto actorPtr = hugger_handle.get();
                if (!actorPtr) {
                    logger::warn("backhug::start: actor lookup failed");
                    return;
                }
                release_assert(form_cache::MiniMolestAnimMarker != nullptr);
                actorPtr->MoveTo(form_cache::MiniMolestAnimMarker);

                // Papyrus sets angle = AngleZ + angle (angle == 0 in original). We reuse AngleZ_raw as-is.
                actorPtr->SetAngle(RE::NiPoint3{ 0.0f, 0.0f, AngleZ_raw });

                // fire animations
                actorPtr->NotifyAnimationGraph("BaboBackHugStartM");
                auto playerLocal = RE::PlayerCharacter::GetSingleton();
                release_assert(playerLocal != nullptr);
                playerLocal->NotifyAnimationGraph("BaboBackHugStartF");
                };

        }*/

    

    }
}
