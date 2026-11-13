ScriptName CRUI_Hud Native Hidden
{Widgets on the HUD without replacing HUDMenu.swf, from CleanroomUI.dll: once a mod adds one, the DLL loads our host movie into whatever HUD movie is installed. Widgets are forgotten as a save loads or a new game starts: add yours on every game load. See docs/swf_framework.md#hud}

Function AddWidget(String asName, String asMovie, Float afX = 0.0, Float afY = 0.0, String asAnchor = "") Global Native
{Show movie asMovie (a path under Interface; any HUDFramework widget, which implements processMessage(command, params)) as widget asName, under HUD part asAnchor (clip names from the HUD's root, dotted, e.g. "LeftMeters_mc", so it moves and fades with that part; "" for the HUD itself) at afX, afY from that part's top-left. A name given again moves or replaces its widget.}

Function AddStats(String asName, String[] asLabels, ActorValue[] akValues, Float[] afSteps, GlobalVariable akShown = None, Float afX = 0.0, Float afY = 0.0, String asAnchor = "") Global Native
{Show a line of stats as widget asName, placed as AddWidget places one: each label (a $KEY is translated) with its actor value on the player, left to right. A value below afSteps[0] is dim, from afSteps[1] it takes the HUD's warning color, from afSteps[2] it flashes; give fewer steps to leave the rest off. Shown while akShown is not 0 (None: always).}

Function RemoveWidget(String asName) Global Native
{Take widget asName off the HUD.}

Function Send(String asName, String asCommand, String[] asParams) Global Native
{Call widget asName's processMessage(asCommand, asParams) on its next frame, at once, with no notification text or throttle; held until its movie has loaded.}
