#include "console.h"

namespace console {

    // Der Callback für deinen Befehl
    bool DispatchFreezeTime(const RE::SCRIPT_PARAMETER*, RE::SCRIPT_FUNCTION::ScriptData*, RE::TESObjectREFR*, RE::TESObjectREFR*, RE::Script*, RE::ScriptLocals*, double&, std::uint32_t&)
    {
        if (auto main = RE::Main::GetSingleton()) {
            auto& runtimeData = main->GetRuntimeData();

            // Flag umschalten
            runtimeData.freezeTime = !runtimeData.freezeTime;

            if (auto console = RE::ConsoleLog::GetSingleton()) {
                console->Print(runtimeData.freezeTime ? "FreezeTime: AKTIVIERT" : "FreezeTime: DEAKTIVIERT");
            }
        }
        return true;
    }

    void RegisterFreezeTimeCommand()
    {
        // Da wir einen reinen Toggle bauen, brauchen wir KEINE 'kCommandParam' Struktur!
        // Wir setzen numParams später einfach auf 0.

        static constexpr std::array kSacrificialCommands = {
            "StartAllQuests",
            "CompleteAllQuestStages",
            "ToggleFogOfWar",
            "ToggleWireframe"
        };

        for (const char* sacrificialName : kSacrificialCommands) {
            // RE::SCRIPT_FUNCTION::LocateConsoleCommand sucht via CommonLibSSE automatisch das Array ab
            if (auto cmd = RE::SCRIPT_FUNCTION::LocateConsoleCommand(sacrificialName)) {

                cmd->functionName = "ToggleFreezeTime";
                cmd->shortName = "tft";
                cmd->helpString = "Schaltet das freezeTime Flag im Main-Singleton um.";
                cmd->referenceFunction = false; // Muss nicht auf ein ausgewähltes Objekt/NPC referenzieren

                cmd->executeFunction = &DispatchFreezeTime;
                cmd->numParams = 0;      // Keine Argumente benötigt
                cmd->params = nullptr;   // Kein Parameter-Array nötig
                return;
            }
        }
    }


}