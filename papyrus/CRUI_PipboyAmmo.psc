ScriptName CRUI_PipboyAmmo Native Hidden
{The Pip-Boy's ammo list, from CleanroomUI.dll (dll/framework/ammo.cpp). See docs/fork/fo4_condition.md#ammo-list}

Function SetLists(Form[] akLists, Int[] aiStarts, Form[] akRounds, Form[] akMods) Global Native
{Give the plugin your ammo lists: akLists the keywords your guns carry, aiStarts each list's first round in akRounds (one more entry: the end), akMods each round's ammo mod. They add to other mods' lists; a list given again replaces its rounds. The plugin forgets them each game start, so call this on every game load. The AMMO hint and B key appear only once some mod has given lists.}

Form Function ChosenList() Global Native
{The list (keyword) of the round chosen in the Pip-Boy; take the choice only when it is one of yours. None with no choice waiting.}

Form Function ChosenGun() Global Native
{The gun whose ammo was chosen in the Pip-Boy (it sends CRUI_AmmoSwitch as the choice is confirmed); None with no choice waiting.}

Int Function TakeChoice() Global Native
{The chosen round's index in your akRounds (the mod whose list ChosenList is), cleared as it is taken; -1 with none.}
