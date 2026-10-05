#include "actor_util.h"

namespace actor_util {
    namespace internal {
        void initialize() {
            RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> typeInfo;
            auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
            if (vm && vm->GetScriptObjectType("ActorUtil", typeInfo) && typeInfo) {
                auto funcIter = typeInfo->GetGlobalFuncIter();
                auto numFuncs = typeInfo->GetNumGlobalFuncs();
                for (std::uint32_t i = 0; i < numFuncs; ++i, ++funcIter) {
                    auto func = funcIter->func;
                    if (func && func->GetIsNative()) {
                        if (std::string(func->GetName()) == "AddPackageOverride") {
                            auto nativeFnAddr = reinterpret_cast<char*>(func.get());
                            char* stdFunctionAddr = nativeFnAddr + 0x50;
                            std::uintptr_t rawFunctionAddress = *reinterpret_cast<std::uintptr_t*>(stdFunctionAddr);
                            if (rawFunctionAddress > 0x00010000) { // Einfacher Validitätscheck für Adressen
                                logger::debug("Found ActorUtil.AddPackageOverride at: {:X}", rawFunctionAddress);
                                add_package_override = reinterpret_cast<add_package_override_t>(rawFunctionAddress);
                            }
                            else {
                                logger::error("ActorUtil.AddPackageOverride address is invalid: {:X}", rawFunctionAddress);
                            }
                        }
                        else if (std::string(func->GetName()) == "RemovePackageOverride") {
                            auto nativeFnAddr = reinterpret_cast<char*>(func.get());
                            char* stdFunctionAddr = nativeFnAddr + 0x50;
                            std::uintptr_t rawFunctionAddress = *reinterpret_cast<std::uintptr_t*>(stdFunctionAddr);
                            if (rawFunctionAddress > 0x00010000) { // Einfacher Validitätscheck für Adressen
                                logger::debug("Found ActorUtil.RemovePackageOverride at: {:X}", rawFunctionAddress);
                                remove_package_override = reinterpret_cast<remove_package_override_t>(rawFunctionAddress);
                            }
                            else {
                                logger::error("ActorUtil.RemovePackageOverride address is invalid: {:X}", rawFunctionAddress);
                            }
                        }
                        else if (std::string(func->GetName()) == "CountPackageOverride") {
                            auto nativeFnAddr = reinterpret_cast<char*>(func.get());
                            char* stdFunctionAddr = nativeFnAddr + 0x50;
                            std::uintptr_t rawFunctionAddress = *reinterpret_cast<std::uintptr_t*>(stdFunctionAddr);
                            if (rawFunctionAddress > 0x00010000) { // Einfacher Validitätscheck für Adressen
                                logger::debug("Found ActorUtil.CountPackageOverride at: {:X}", rawFunctionAddress);
                                count_package_override = reinterpret_cast<count_package_override_t>(rawFunctionAddress);
                            }
                            else {
                                logger::error("ActorUtil.CountPackageOverride address is invalid: {:X}", rawFunctionAddress);
                            }
                        }
                    }
                }
            }
            if (!add_package_override || !remove_package_override) {
                logger::error("ActorUtil initialization failed.");
            }
        }
    }
}
