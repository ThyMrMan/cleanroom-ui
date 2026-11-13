// CleanroomDialogue.dll: our dialogue menu in place of the game's or XDI's (dialogue.cpp), on its own.
#pragma once

#include "gfx.h"

namespace crui {

// Point the dialogue option calls and the Player Dialogue run at our menu's options, through g_branchTrampoline (set
// up); false when XDI.dll is loaded or a call site no longer calls the vanilla function. Run once plugins have loaded.
// `tasks` (F4SE's task interface) runs the menu's leave on the game's thread.
bool HookDialogue(void *tasks);

// F4SE's Scaleform callback: give a movie's f4se.plugins.CleanroomDialogue the menu natives and the shared ones.
bool RegisterDialogueMenu(GFxMovieView *view, GFxValue *plugin);

}  // namespace crui
