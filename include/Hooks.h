#pragma once

#include "Config.h"

#define M_PI 3.14159265358979323846f

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
            auto start = std::chrono::high_resolution_clock::now();
            // Respect existing
            if (HasWindModifier(a_this)) {
                return;
            }
            float strength = Config::GetSingleton()->GetStrength(a_this);
            // Zero means ignore (negative values are accepted)
            if (strength == 0.0f) {
                return;
            }
            // Add Wind
            auto wind = RE::BSWindModifier::Create("ParticleWind", strength);
            if (!wind) return;
            a_this->AddModifier(wind);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end - start;
            logger::debug("This took {} ms", elapsed.count());
        }

        static inline REL::Relocation<decltype(Hook)> original;
    };

    struct TESUpdateBsWindModifierWindVectorHook {
        static void UpdateBsWindModifierWindVector(float strength, float angle) {
            return original(strength, angle - (M_PI / 2.0f));
        }
        static inline REL::Relocation<decltype(UpdateBsWindModifierWindVector)> original;
    };

    inline void InstallHooks() {
        REL::Relocation<std::uintptr_t> vtblNiPS{RE::VTABLE_NiParticleSystem[0]};

        LinkObjectHook::original = vtblNiPS.write_vfunc(0x19, LinkObjectHook::Hook);

        if (Config::GetSingleton()->windDirectionFix_) {
            auto& trampoline = SKSE::GetTrampoline();
            SKSE::AllocTrampoline(14);
            TESUpdateBsWindModifierWindVectorHook::original =
                trampoline.write_call<5>(REL::RelocationID(13151, 13291).address() + REL::Relocate(0x8E, 0x8A),
                                         TESUpdateBsWindModifierWindVectorHook::UpdateBsWindModifierWindVector);
        }
    }
}