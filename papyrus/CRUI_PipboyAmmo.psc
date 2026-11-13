ScriptName CRUI_PipboyAmmo Native Hidden
{The Pip-Boy's ammo list, from CleanroomUI.dll (dll/framework/ammo.cpp). See docs/fork/fo4_condition.md#ammo-list}

Function SetLists(Form[] akLists, Int[] aiStarts, Form[] akRounds, Form[] akMods) Global Native
{Give the plugin F3NV_AmmoSwitch's ammo lists (its arrays of the same names); it forgets them each game start.}

Form Function ChosenGun() Global Native
{The gun whose ammo was chosen in the Pip-Boy (it sends CRUI_AmmoSwitch as the choice is confirmed); None with no choice waiting.}

Int Function TakeChoice() Global Native
{The chosen round's index in F3NV_AmmoSwitch.Rounds, cleared as it is taken; -1 with none.}
