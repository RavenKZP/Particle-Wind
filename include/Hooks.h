#pragma once


namespace Hooks {

    struct NiParticleSystem_AddModifier_Hook {
        static void Hook(RE::NiParticleSystem* a_this, RE::NiStream& a_stream) {
            originalFunc_(a_this, a_stream);
            static auto wind = RE::BSWindModifier::Create("ParticleWind", 10.0f);
            if (!wind) return;
            a_this->AddModifier(wind);
        }

        static inline REL::Relocation<decltype(Hook)> originalFunc_;
    };

    inline void InstallHooks() {
        REL::Relocation<std::uintptr_t> vtbl{RE::VTABLE_NiParticleSystem[0]};
        NiParticleSystem_AddModifier_Hook::originalFunc_ =
            vtbl.write_vfunc(0x18, NiParticleSystem_AddModifier_Hook::Hook);  // LoadBinary
    }
}