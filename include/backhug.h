#pragma once

namespace animation {

    namespace backhug {
        struct backhug_data {
            RE::NiPoint3 markerTarget;
            RE::NiPoint3 markerRot;
            float AngleZ_raw;
        };
        backhug_data backhug_start(RE::Actor* a_actor);
        void backhug_after_500ms(RE::NiPoint3 markerTarget, RE::NiPoint3 markerRot);
        void backhug_after_1000ms(RE::ActorPtr actorPtr, float AngleZ_raw);
        void backhug_stop_actor(RE::Actor* a_actor);
        void backhug_stop_player();
    }



}
