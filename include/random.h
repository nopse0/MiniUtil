#pragma once

namespace random {

    class random : public singleton<random> {
    private:
        std::mt19937 _generator;

    public:
        random();

        void reset();
        void new_game();
        void serialize(SKSE::SerializationInterface* a_serde);
        uint32_t deserialize(SKSE::SerializationInterface* a_serde);

        float gen_float(float a_min, float a_max);
        int gen_int(int a_min, int a_max);
    };

}
