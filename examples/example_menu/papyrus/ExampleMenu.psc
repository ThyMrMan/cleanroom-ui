ScriptName ExampleMenu extends Quest
{An example menu on Cleanroom UI (Interface/ExampleMenu.swf). Attach it to a quest of your plugin that starts with the game; open the menu with ExampleMenu.Open() (from the console: cgf "ExampleMenu.Open"). The movie sends ExampleMenu_Pick (the item's index) and ExampleMenu_Close as F4SE external events, which this quest listens for. F4SE forgets those registrations each load, so they are made again on every game load. Rename ExampleMenu everywhere to make it yours. See docs/api.md#example}

String Property MenuName = "ExampleMenu" AutoReadOnly Hidden

Event OnQuestInit()
  Listen()
EndEvent

Event Actor.OnPlayerLoadGame(Actor akSender)
  Listen()
EndEvent

Function Listen()
  {Register the menu and listen for its events; on start and on every game load.}
  RegisterForRemoteEvent(Game.GetPlayer(), "OnPlayerLoadGame")
  RegisterMenu()
  UnregisterForExternalEvent("ExampleMenu_Pick")
  UnregisterForExternalEvent("ExampleMenu_Close")
  RegisterForExternalEvent("ExampleMenu_Pick", "OnPick")
  RegisterForExternalEvent("ExampleMenu_Close", "OnClose")
EndFunction

Function RegisterMenu() Global
  {Register the menu with F4SE once: it pauses the game, shows the cursor and takes the menu controls (a pad too), in the HUD's colors.}
  If UI.IsMenuRegistered("ExampleMenu")
    Return
  EndIf
  UI:MenuData data = new UI:MenuData
  data.menuFlags = 0x800049D
  data.extendedFlags = 2
  UI.RegisterCustomMenu("ExampleMenu", "ExampleMenu", "root1", data)
EndFunction

Function Open() Global
  {Open the menu.}
  RegisterMenu()
  UI.OpenMenu("ExampleMenu")
EndFunction

Function OnPick(Int aiIndex)
  {An item was picked: close the menu and say which.}
  UI.CloseMenu(MenuName)
  Debug.Notification("You picked item " + (aiIndex + 1))
EndFunction

Function OnClose()
  {Back was pressed.}
  UI.CloseMenu(MenuName)
EndFunction
