ScriptName CRUI_HudNative Native Hidden
{CleanroomUI.dll's HUD widget list, for CRUI_Hud: call CRUI_Hud, not these. See docs/swf_framework.md#hud}

Function AddWidget(String asName, String asMovie, Float afX, Float afY, String asAnchor) Global Native
{Widget asName: movie asMovie, under HUD part asAnchor at afX, afY.}

Function AddStats(String asName, String[] asLabels, ActorValue[] akValues, Float[] afSteps, GlobalVariable akShown, Float afX, Float afY, String asAnchor) Global Native
{Stats line asName; see CRUI_Hud.AddStats.}

Function RemoveWidget(String asName) Global Native
{Take widget asName off the list.}

Function Send(String asName, String asCommand, String[] asParams) Global Native
{Hand widget asName processMessage(asCommand, asParams) on its next frame.}

Function SetCondition(String asName, Bool abOn, String asLabel) Global Native
{Condition asName for the HUD's show rules holds (abOn) or not; see CRUI_Hud.SetCondition.}

Function SetConditionGlobal(String asName, GlobalVariable akValue, String asLabel) Global Native
{Condition asName holds while akValue is not 0; see CRUI_Hud.SetConditionGlobal.}

Function RemoveCondition(String asName) Global Native
{Take condition asName off.}

Function SetShowRule(String asName, String asRule) Global Native
{Box asName's rule from its mod; see CRUI_Hud.SetShowRule.}
