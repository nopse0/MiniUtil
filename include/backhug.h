#pragma once

namespace animation {

    /*struct backhug {
        static constexpr float _sleep1_duration_seconds = 0.5f;
        static constexpr float _sleep2_duration_seconds = 0.5f;

        //RE::ActorHandle _hugger_handle;
        //std::function<void()> _sleep1;
        //std::function<void()> _sleep2;

        bool start(RE::Actor* a_actor);
        void stop(RE::Actor* a_actor);
    };*/

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
