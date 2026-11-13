// CleanroomDialogue.dll: our dialogue menu in place of the game's or XDI's (dialogue.cpp), on its own.
#pragma once

#include "base.h"

namespace crui {

// Point the dialogue option calls and the Player Dialogue run at our menu's options, through CommonLibF4's trampoline
// (reserved at F4SE::Init); false when XDI.dll is loaded, the runtime is not the one the layouts were read in, or a
// call site no longer calls the vanilla function. Run once plugins have loaded.
bool HookDialogue();

// F4SE's Scaleform callback: give a movie's f4se.plugins.CleanroomDialogue the menu natives and the shared ones.
bool RegisterDialogueMenu(Scaleform::GFx::Movie *view, Scaleform::GFx::Value *plugin);

}  // namespace crui
