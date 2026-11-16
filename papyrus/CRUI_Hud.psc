ScriptName CRUI_Hud Hidden
{Widgets on the HUD without replacing HUDMenu.swf: CleanroomUI.dll loads our host movie into whatever HUD movie is installed, and the player moves, scales and hides each widget in our edit menu (EditLayout). A widget is forgotten as a save loads or a new game starts: add yours on every game load. See docs/swf_framework.md#hud}

Function AddWidget(String asName, String asMovie, Float afX = 0.0, Float afY = 0.0, String asAnchor = "") Global
{Show movie asMovie (a path under Interface; a widget movie, which implements processMessage(command, params)) as widget asName, under HUD part asAnchor (clip names from the HUD's root, dotted, e.g. "LeftMeters_mc", so it moves and fades with that part; "" for the HUD itself) at afX, afY from that part's top-left. A name given again moves or replaces its widget.}
  CRUI_HudNative.AddWidget(asName, asMovie, afX, afY, asAnchor)
EndFunction

Function AddStats(String asName, String[] asLabels, ActorValue[] akValues, Float[] afSteps, GlobalVariable akShown = None, Float afX = 0.0, Float afY = 0.0, String asAnchor = "") Global
{Show a line of stats as widget asName, placed as AddWidget places one: each label (a $KEY is translated) with its actor value on the player, left to right. A value below afSteps[0] is dim, from afSteps[1] it takes the HUD's warning color, from afSteps[2] it flashes; give fewer steps to leave the rest off. Shown while akShown is not 0 (None: always).}
  CRUI_HudNative.AddStats(asName, asLabels, akValues, afSteps, akShown, afX, afY, asAnchor)
EndFunction

Function RemoveWidget(String asName) Global
{Take widget asName off the HUD.}
  CRUI_HudNative.RemoveWidget(asName)
EndFunction

Function Send(String asName, String asCommand, String[] asParams) Global
{Call widget asName's processMessage(asCommand, asParams) on its next frame; held until the widget has loaded.}
  CRUI_HudNative.Send(asName, asCommand, asParams)
EndFunction

Function SetCondition(String asName, Bool abOn, String asLabel = "") Global
{A condition of your own for the HUD's show rules (docs/swf_framework.md#hud-conditions): a box whose rule names asName (letters, digits, _ and . only; not one of ours, such as combat) shows while it holds. asLabel is how the edit menu's list shows it (a $KEY is translated; "" keeps the last one given, else asName). Forgotten as a save loads: set yours on every game load. A rule naming a condition nobody set counts it as holding, so a box never hides for good when your mod is gone.}
  CRUI_HudNative.SetCondition(asName, abOn, asLabel)
EndFunction

Function SetConditionGlobal(String asName, GlobalVariable akValue, String asLabel = "") Global
{Condition asName holds while akValue is not 0, read each frame with no Papyrus call: for a state your mod keeps in a global anyway. asLabel as SetCondition.}
  CRUI_HudNative.SetConditionGlobal(asName, akValue, asLabel)
EndFunction

Function RemoveCondition(String asName) Global
{Take condition asName off.}
  CRUI_HudNative.RemoveCondition(asName)
EndFunction

Function SetShowRule(String asName, String asRule) Global
{When box asName shows, unless the player set its rule in the edit menu: asRule is its conditions, comma separated (e.g. "combat,mymod.alarm,peek"; "always" for always; low:0.3 for a threshold), over the preset's ([Hud] sShow). asName is your widget's name, or "part:" and one of the HUD's parts (e.g. "part:CompassWidget_mc"). "" takes yours off. Forgotten as a save loads: set it on every game load.}
  CRUI_HudNative.SetShowRule(asName, asRule)
EndFunction

Function EditLayout() Global
{Open the edit menu for the widgets on our host and the game's own HUD parts: drag a box to move its widget or part, turn the mouse wheel over it to scale it, R (a setting in CleanroomUI.ini) puts it back where its mod or the game puts it, Esc or Tab saves and closes. The places are kept for every save, in Documents\My Games\Fallout4\F4SE\CleanroomUI_Hud.txt. For an MCM button, or the console: cgf "CRUI_Hud.EditLayout".}
  String menu = "CRUI_HudEdit"
  If !UI.IsMenuRegistered(menu)
    UI:MenuData data = new UI:MenuData
    data.menuFlags = 0x800049D
    data.extendedFlags = 2
    UI.RegisterCustomMenu(menu, menu, "root1", data)
  EndIf
  UI.OpenMenu(menu)
EndFunction
