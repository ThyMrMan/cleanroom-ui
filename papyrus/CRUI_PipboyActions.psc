ScriptName CRUI_PipboyActions Hidden
{The Pip-Boy's action menu on a pad (Interface/CRUI_PipboyActions.swf): CleanroomUI.dll calls Open as View is pressed in the Pip-Boy while a mod uses Repair or Ammo. See docs/swf_framework.md#pipboy-pad}

Function Open() Global
{Open the action menu over the Pip-Boy, registering it first: no cursor, the game paused, the pad's menu controls, above the Pip-Boy (depth 10), in the HUD's colors.}
  String menu = "CRUI_PipboyActions"
  If !UI.IsMenuRegistered(menu)
    UI:MenuData data = new UI:MenuData
    data.menuFlags = 0x8000099
    data.extendedFlags = 2
    data.depth = 10
    UI.RegisterCustomMenu(menu, menu, "root1", data)
  EndIf
  UI.OpenMenu(menu)
EndFunction
