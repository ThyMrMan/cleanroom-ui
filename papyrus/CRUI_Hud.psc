ScriptName CRUI_Hud Hidden
{Widgets on the HUD without replacing HUDMenu.swf. With HUDFramework installed it hosts them, so players move, scale and fade them with its own editor and it keeps their places; without it CleanroomUI.dll loads our host movie into whatever HUD movie is installed. Either way a widget is forgotten as a save loads or a new game starts: add yours on every game load. HUDFramework is reached through Utility.CallGlobalFunction and CallFunction, so nothing here needs its scripts. See docs/swf_framework.md#hud}

Function AddWidget(ScriptObject akOwner, String asName, String asMovie, Float afX = 0.0, Float afY = 0.0, String asAnchor = "") Global
{Show movie asMovie (a path under Interface; any HUDFramework widget, which implements processMessage(command, params)) as widget asName. On our host it sits under HUD part asAnchor (clip names from the HUD's root, dotted, e.g. "LeftMeters_mc", so it moves and fades with that part; "" for the HUD itself) at afX, afY from that part's top-left. Under HUDFramework it starts at afX, afY in HUDFramework's own place and the player moves it; akOwner is its owner there (HUDFramework drops a widget whose owner is gone, and calls akOwner's HUD_WidgetLoaded(asWidgetID) when it loads). A name given again moves or replaces its widget.}
  ScriptObject hud = Framework()
  String host = ""
  If hud
    Register(hud, akOwner, asMovie, afX, afY)
    host = "HUDFramework"
  EndIf
  CRUI_HudNative.AddWidget(asName, asMovie, afX, afY, asAnchor, host)
EndFunction

Function AddStats(ScriptObject akOwner, String asName, String[] asLabels, ActorValue[] akValues, Float[] afSteps, GlobalVariable akShown = None, Float afX = 0.0, Float afY = 0.0, String asAnchor = "") Global
{Show a line of stats as widget asName, placed as AddWidget places one: each label (a $KEY is translated) with its actor value on the player, left to right. A value below afSteps[0] is dim, from afSteps[1] it takes the HUD's warning color, from afSteps[2] it flashes; give fewer steps to leave the rest off. Shown while akShown is not 0 (None: always). Under HUDFramework every stats line is drawn by our one stats widget (Interface/CRUI_HudStats.swf), one line under another, which the player places.}
  ScriptObject hud = Framework()
  String host = ""
  If hud
    Register(hud, akOwner, "CRUI_HudStats.swf", afX, afY)
    host = "HUDFramework"
  EndIf
  CRUI_HudNative.AddStats(asName, asLabels, akValues, afSteps, akShown, afX, afY, asAnchor, host)
EndFunction

Function RemoveWidget(String asName) Global
{Take widget asName off the HUD (under HUDFramework, its registration too, with the place the player gave it).}
  String movie = CRUI_HudNative.MovieOf(asName)
  ScriptObject hud = Framework()
  If hud && CRUI_HudNative.HostOf(asName) == "HUDFramework" && movie != ""
    Var[] args = new Var[1]
    args[0] = movie
    hud.CallFunction("UnloadWidget", args)
    hud.CallFunction("UnregisterWidget", args)
  EndIf
  CRUI_HudNative.RemoveWidget(asName)
EndFunction

Function Send(String asName, String asCommand, String[] asParams) Global
{Call widget asName's processMessage(asCommand, asParams). On our host it runs on the widget's next frame; under HUDFramework it goes through HUDFramework's own message channel (on PC about once a second, a newer message for the same command replacing an older). Held until the widget has loaded.}
  String host = CRUI_HudNative.HostOf(asName)
  ScriptObject hud = None
  If host == "HUDFramework"
    hud = Framework()
  EndIf
  If !hud
    CRUI_HudNative.Send(asName, asCommand, asParams)
    Return
  EndIf
  Var[] args = new Var[3]
  args[0] = CRUI_HudNative.MovieOf(asName)
  args[1] = asCommand
  args[2] = Joined(asParams)
  hud.CallFunction("SendMessageString", args)
EndFunction

ScriptObject Function Framework() Global
{HUDFramework's quest once it is ready, when HUDFramework.esm (or .esp) is installed; None otherwise.}
  If !Game.IsPluginInstalled("HUDFramework.esm") && !Game.IsPluginInstalled("HUDFramework.esp")
    Return None
  EndIf
  Return Utility.CallGlobalFunction("HUDFramework", "GetInstance", new Var[0]) as ScriptObject
EndFunction

Function Register(ScriptObject akFramework, ScriptObject akOwner, String asMovie, Float afX, Float afY) Global
{Register movie asMovie with HUDFramework, loaded now and on every game load, only when it is not registered yet: registering again would put back afX, afY over the place the player gave it.}
  Var[] id = new Var[1]
  id[0] = asMovie
  If akFramework.CallFunction("IsWidgetRegistered", id) as Bool
    Return
  EndIf
  Var[] args = new Var[6]
  args[0] = akOwner
  args[1] = asMovie
  args[2] = afX
  args[3] = afY
  args[4] = True
  args[5] = True
  akFramework.CallFunction("RegisterWidget", args)
EndFunction

String Function Joined(String[] asParams) Global
{asParams as a HUDFramework message body: joined by " | ", which it splits back into processMessage's params.}
  String out = ""
  Int i = 0
  While asParams && i < asParams.Length
    If i > 0
      out += " | "
    EndIf
    out += asParams[i]
    i += 1
  EndWhile
  Return out
EndFunction
