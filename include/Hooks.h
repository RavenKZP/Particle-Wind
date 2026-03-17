#pragma once


namespace Hooks {

    static bool HasWindModifier(RE::NiParticleSystem* ps) {
        if (!ps) return false;
        auto& list = ps->GetParticleSystemRuntimeData().modifierList;
        for (const auto& mod : list) {
            if (skyrim_cast<RE::BSWindModifier*>(mod.get())) {
                return true;
            }
        }
        return false;
    }

    struct LinkObjectHook {
        static void Hook(RE::NiParticleSystem* a_this, RE::NiStream& a_stream) {
            original(a_this, a_stream);
            if (!HasWindModifier(a_this)) {
                auto wind = RE::BSWindModifier::Create("ParticleWind", 10.0f);
                if (!wind) return;
                a_this->AddModifier(wind);
            }
        }

        static inline REL::Relocation<decltype(Hook)> original;
    };

    inline void InstallHooks() {
        REL::Relocation<std::uintptr_t> vtbl{RE::VTABLE_NiParticleSystem[0]};
        LinkObjectHook::original = vtbl.write_vfunc(0x19, LinkObjectHook::Hook);
    }
}