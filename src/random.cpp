#include "random.h"

namespace random {

    random::random() {
        reset();
    }

    void random::reset() {
        // Fallback-Initialisierung, falls kein Savegame geladen wird
        // (Nutzt die Systemzeit, um das langsame random_device beim Start zu umgehen)
        logger::debug("random::reset()");
        std::uint32_t timeSeed = static_cast<std::uint32_t>(std::time(nullptr));
        _generator.seed(timeSeed);
    }

    // Holt eine Zufallszahl (float) in einem bestimmten Bereich
    float  random::gen_float(float a_min, float a_max) {
        std::uniform_real_distribution<float> distrib(a_min, a_max);
        return distrib(_generator);
    }

    // Holt eine Zufallszahl (int) in einem bestimmten Bereich
    int  random::gen_int(int a_min, int a_max) {
        std::uniform_int_distribution<int> distrib(a_min, a_max);
        return distrib(_generator);
    }

    // --- SERIALISIERUNG ---

    // `std::mt19937` lässt sich direkt in einen Stream schreiben!
    // Dadurch wird sein kompletter interner Zustand (die gesamte Zustandskette) als Text exportiert.
    void  random::serialize(SKSE::SerializationInterface* a_serde) {
        std::stringstream ss;
        ss << _generator; // Schreibt den kompletten Zustand als String in den Stream

        std::string stateStr = ss.str();
        std::uint32_t size = static_cast<std::uint32_t>(stateStr.length());

        // 1. Länge des Zustands-Strings schreiben
        a_serde->WriteRecordData(&size, sizeof(size));
        // 2. Den String selbst schreiben
        a_serde->WriteRecordData(stateStr.data(), size);
    }

    // Den Zustand aus dem Savegame wiederherstellen
    uint32_t random::deserialize(SKSE::SerializationInterface* a_serde) {
        std::uint32_t size = 0;
        a_serde->ReadRecordData(&size, sizeof(size));

        std::string stateStr;
        stateStr.resize(size);
        a_serde->ReadRecordData(stateStr.data(), size);

        std::stringstream ss(stateStr);
        ss >> _generator; // Stellt den exakten Zustand des Generators wieder her!

        return size;
    }

    // Falls ein neues Spiel gestartet wird: Hier darf random_device einmalig laufen
    void  random::new_game() {
        logger::debug("random::new_game()");
        std::random_device rd;
        _generator.seed(rd());
    }
}
