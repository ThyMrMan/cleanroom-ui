ScriptName CRUI_ItemCard Native Hidden
{Lines of your own on the Pip-Boy's inventory item card, from CleanroomUI.dll (dll/framework/card.cpp), drawn by our Pip-Boy movie under any Pip-Boy that keeps the vanilla item card. Lines are forgotten when a save loads or a new game starts: set them again on each game load (OnPlayerLoadGame). See docs/swf_framework.md#card-lines}

Function SetLine(Form akItem, String asLabel, String asValue) Global Native
{Show asLabel with asValue on akItem's item card (a Keyword: on the card of every item carrying it), replacing that item's line of the same label. An empty asValue removes the line.}
