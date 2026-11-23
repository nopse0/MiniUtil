#include "state_all.h"
#include "detours/detours.h"
#include "serialization.h"

namespace serialization {
    namespace save_blocker {
        namespace detail {
            struct SaveImplHook
            {
                static bool thunk(
                    RE::BGSSaveLoadManager* a_this,
                    std::int32_t            a_deviceID,
                    std::uint32_t           a_outputStats,
                    const char* a_fileName)
                {
                    if (is_blocked()) {
                        logger::info("SaveImplHook: Saving was blocked");
                        // On screen notification
                        if(auto* tasks = SKSE::GetTaskInterface()) {
                            tasks->AddTask([]() {
                                RE::SendHUDMessage::ShowHUDMessage("You cannot save right now.");
                                });
                        }
                        return false;
                    }
                    return func(a_this, a_deviceID, a_outputStats, a_fileName);
                }
                static inline REL::Relocation<decltype(thunk)> func;
            };

            void install_hook() {
                auto targetAddr = REL::Relocation<std::uintptr_t>{ RELOCATION_ID(34818, 35727) }.address();
                detail::SaveImplHook::func = targetAddr;

                DetourTransactionBegin();
                DetourUpdateThread(GetCurrentThread());
                DetourAttach(&(PVOID&)detail::SaveImplHook::func, detail::SaveImplHook::thunk);
                DetourTransactionCommit();
            }
        }
    }

    bool SaveString(SKSE::SerializationInterface* a_intfc, const RE::BSFixedString& a_string)
    {
        // 1. Get the length of the string
        uint32_t length = a_string.size();

        // 2. Write the length first
        if (!a_intfc->WriteRecordData(&length, sizeof(length))) {
            // Handle error (e.g., log it)
            return false;
        }

        // 3. Write the actual string data if it's not empty
        if (length > 0) {
            if (!a_intfc->WriteRecordData(a_string.c_str(), length)) {
                // Handle error
                return false;
            }
        }

        return true;
    }

    bool SaveNiPoint3(SKSE::SerializationInterface* a_intfc, const RE::NiPoint3& a_point) {
        return a_intfc->WriteRecordData(&a_point.x, sizeof(a_point.x))
            && a_intfc->WriteRecordData(&a_point.y, sizeof(a_point.y))
            && a_intfc->WriteRecordData(&a_point.z, sizeof(a_point.z));
    }

    int32_t LoadString(SKSE::SerializationInterface* a_intfc, RE::BSFixedString& a_outString)
    {
        int32_t read_bytes = 0;
        uint32_t current_read = 0;
        
        // 1. Read the length
        uint32_t length = 0;
        current_read = a_intfc->ReadRecordData(&length, sizeof(length));
        if (current_read == sizeof(length)) read_bytes += current_read; else return -1;

        // 2. If length is 0, assign an empty string and exit
        if (length == 0) {
            a_outString = "";
            return read_bytes;
        }

        // 3. Create a buffer to hold the data (+1 for the null terminator)
        std::string buffer;
        buffer.resize(length);

        // 4. Read the character data into the buffer
        current_read = a_intfc->ReadRecordData(&buffer[0], length);
        if (current_read == length) read_bytes += current_read; else return -1;

        // 5. Assign it to the BSFixedString (this automatically adds it to Skyrim's internal string table)
        a_outString = buffer;
        return read_bytes;
    }

    bool SaveActorHandle(SKSE::SerializationInterface* a_serde, const RE::ActorHandle& a_handle) {
        RE::FormID formId = 0;
        auto actorPtr = a_handle.get();
        if (actorPtr) {
            formId = actorPtr->GetFormID();
        }
        return a_serde->WriteRecordData(&formId, sizeof(formId));
    }

    int32_t LoadActorHandle(SKSE::SerializationInterface* a_serde, RE::ActorHandle& a_handle) {
        int32_t read_bytes = 0;
        uint32_t current_read = 0;

        a_handle.reset();

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
                    a_handle = refr->GetHandle();
                }
            }
        }

        return read_bytes;
    }

    int32_t LoadNiPoint3(SKSE::SerializationInterface* a_intfc, RE::NiPoint3& a_point) {
        int32_t read_bytes = 0;
        uint32_t current_read = 0;
        float x, y, z;
        current_read = a_intfc->ReadRecordData(&x, sizeof(x));
        if (current_read == sizeof(x)) read_bytes += current_read; else return -1;
        current_read = a_intfc->ReadRecordData(&y, sizeof(y));
        if (current_read == sizeof(y)) read_bytes += current_read; else return -1;
        current_read = a_intfc->ReadRecordData(&z, sizeof(z));
        if (current_read == sizeof(z)) read_bytes += current_read; else return -1;
        a_point = RE::NiPoint3(x, y, z);
        return read_bytes;
    }

    int32_t LoadFreeState(SKSE::SerializationInterface* a_serde, fsm::fsm_state& out_state) {
        uint32_t loaded_state_type = 0;
        int32_t read_bytes = 0;
        
        read_bytes = a_serde->ReadRecordData(&loaded_state_type, sizeof(loaded_state_type));
        if (read_bytes != sizeof(loaded_state_type)) {
            logger::critical("Fehler beim Laden des FSM - Records: Unerwartetes Dateiende!");
            out_state = std::monostate{};
            return -1;
        }

        // 2. Der Switch bestimmt, WELCHE Daten jetzt aus dem Savegame gelesen werden müssen
        switch (loaded_state_type) {

        case static_cast<uint32_t>(StateId::None):
            out_state = std::monostate{};
            return read_bytes;

        case static_cast<uint32_t>(StateId::Idle):
            out_state = fsm::state_idle{};
            return read_bytes;
        }

        out_state = std::monostate{};
        return -1;
    }
}
