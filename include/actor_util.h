#pragma once

namespace actor_util {
    namespace internal {
        using add_package_override_t = void(*)(void*, RE::Actor*, RE::TESPackage*, std::uint32_t, std::uint32_t);
        inline add_package_override_t add_package_override = nullptr;
        using remove_package_override_t = bool(*)(void*, RE::Actor*, RE::TESPackage*);
        inline remove_package_override_t remove_package_override = nullptr;
        using count_package_override_t = std::uint32_t(*)(void*, RE::Actor*);
        inline count_package_override_t count_package_override = nullptr;
        void initialize();
    }

    inline void add_package_override(RE::Actor* a_actor, RE::TESPackage* a_package, std::uint32_t a_priority, std::uint32_t a_flags)
    {
        if (internal::add_package_override) {
            internal::add_package_override(nullptr, a_actor, a_package, a_priority, a_flags);
        }
    }

    inline void remove_package_override(RE::Actor* a_actor, RE::TESPackage* a_package)
    {
        if (internal::remove_package_override) {
            internal::remove_package_override(nullptr, a_actor, a_package);
        }
    }
    
    inline std::uint32_t count_package_override(RE::Actor* a_actor)
    {
		return internal::remove_package_override ? internal::count_package_override(nullptr, a_actor) : 0;
    }

}
