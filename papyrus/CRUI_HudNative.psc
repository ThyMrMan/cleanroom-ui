ScriptName CRUI_HudNative Native Hidden
{CleanroomUI.dll's HUD widget list, for CRUI_Hud, which picks each widget's host: call CRUI_Hud, not these. See docs/swf_framework.md#hud}

Function AddWidget(String asName, String asMovie, Float afX, Float afY, String asAnchor, String asHost) Global Native
{Widget asName (movie asMovie, under HUD part asAnchor at afX, afY) on host asHost: "" ours (the DLL loads our host movie into the HUD), "HUDFramework" (CRUI_Hud has registered it there).}

Function AddStats(String asName, String[] asLabels, ActorValue[] akValues, Float[] afSteps, GlobalVariable akShown, Float afX, Float afY, String asAnchor, String asHost) Global Native
{Stats line asName (see CRUI_Hud.AddStats) on host asHost: "" ours, "HUDFramework" (drawn by our stats widget, which CRUI_Hud has registered there).}

Function RemoveWidget(String asName) Global Native
{Take widget asName off the list.}

Function Send(String asName, String asCommand, String[] asParams) Global Native
{Hand our host's widget asName processMessage(asCommand, asParams) on its next frame.}

String Function HostOf(String asName) Global Native
{Widget asName's host: "" ours, "HUDFramework"; "none" for no such widget.}

String Function MovieOf(String asName) Global Native
{Widget asName's movie; "" for a stats line or no such widget.}

Function SetCondition(String asName, Bool abOn, String asLabel) Global Native
{Condition asName for the HUD's show rules holds (abOn) or not; see CRUI_Hud.SetCondition.}

Function SetConditionGlobal(String asName, GlobalVariable akValue, String asLabel) Global Native
{Condition asName holds while akValue is not 0; see CRUI_Hud.SetConditionGlobal.}

Function RemoveCondition(String asName) Global Native
{Take condition asName off.}

Function SetShowRule(String asName, String asRule) Global Native
{Box asName's rule from its mod; see CRUI_Hud.SetShowRule.}
