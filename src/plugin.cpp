#include "Hooks.h"

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

SKSEPluginLoad(const SKSE::LoadInterface *skse) {
    SKSE::Init(skse);
    Hooks::InstallHooks();
    return true;
}
