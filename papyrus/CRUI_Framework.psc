ScriptName CRUI_Framework Native Hidden
{Cleanroom UI's framework itself, from CleanroomUI.dll. See docs/spin_offs.md#framework-rules}

Int Function GetVersion() Global Native
{The framework's version, raised whenever a native, event or rule changes: check it is at least the one you built against. 0 without CleanroomUI.dll (the call fails).}
