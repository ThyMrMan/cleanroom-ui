ScriptName CRUI_LevelUpNative Native Hidden
{The level-up's natives, from CleanroomUI.dll (framework version 17). See docs/level_up_plan.md#game-points}

Function SetGamePerkPoints(Int aiPoints) Global Native
{Set the game's own perk points, which the Pip-Boy's LEVEL UP notice shows, to aiPoints (0 to 255), on the game's next frame. Game.AddPerkPoints can only add.}
