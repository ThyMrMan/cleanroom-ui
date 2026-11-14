// The game's own perk points, for a level-up of a mod's own (docs/level_up_plan.md#game-points).
//
// FO4 adds a perk point at each level and the Pip-Boy's LEVEL UP notice shows that count (the PlayerInfo
// PerkPoints of pipboymenu.swf); a level-up with points of its own never spends it. Papyrus can only add to it
// (Game.AddPerkPoints, Fallout4.exe 1.11.240 RVA 0x1121690, returns unless the amount is above 0). The game's
// setter, which that native ends in, is PlayerCharacter::SetPerkCount (1.11.240 RVA 0xD75FF0): it writes the count
// at PlayerCharacter +0xCF1 and sends the game's perk points event, which the Pip-Boy reads. The native
// CRUI_LevelUpNative.SetGamePerkPoints queues that setter on the main thread.
#include "condition.h"

#include <algorithm>

namespace crui {

namespace {

const char *LEVEL_UP_SCRIPT = "CRUI_LevelUpNative";

// Papyrus: the game's perk points set to `points` (0 to 255) on the main thread's next task.
void SetGamePerkPoints(std::monostate, std::int32_t points) {
    auto count = static_cast<std::uint8_t>(std::clamp(points, 0, 255));
    if (Tasks()) Tasks()->AddTask([count] {
        if (RE::PlayerCharacter *player = RE::PlayerCharacter::GetSingleton()) player->SetPerkCount(count);
    });
}

}  // namespace

void RegisterPerks(RE::BSScript::IVirtualMachine *vm) {
    // Callable without waiting (F4SE's NoWait flag): it only queues a task.
    vm->BindNativeMethod(LEVEL_UP_SCRIPT, "SetGamePerkPoints", SetGamePerkPoints, true);
}

}  // namespace crui
