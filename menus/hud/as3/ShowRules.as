package
{
   import flash.utils.getTimer;

   // Whether a box (a widget of ours, or one of the game's HUD parts) is shown now by its rule: the conditions it
   // shows under, comma separated, from its place (`show`, the player's or the preset's; CleanroomUI.dll's
   // ShowRule). With no rule it is always shown. A box shows while any of its conditions holds (HudState, read once
   // a frame) and for the linger after the last one ends, so it doesn't flicker. A condition the DLL doesn't know
   // counts as holding (logged once), so a typo never hides a box for good; so does every rule without the DLL.
   // See docs/swf_framework.md#hud-conditions
   public class ShowRules
   {
      private var state:Object;
      // Box name -> when its rule last held (ms, getTimer).
      private var held:Object = {};
      private var log:Function;
      private var seen:Object = {};

      // `log` takes a line.
      public function ShowRules(log:Function)
      {
         this.log = log;
      }

      // The conditions now, from `dll` (HudState), once a frame.
      public function update(dll:Object) : void
      {
         state = dll != null && dll.HudState != null ? dll.HudState() : null;
      }

      // Whether box `name` under rule `rule` ("" or null: always) is shown now.
      public function shows(name:String, rule:String) : Boolean
      {
         if (rule == null || rule == "" || state == null)
         {
            return true;
         }
         var now:int = getTimer();
         var holds:Boolean = false;
         for each (var condition:String in rule.split(","))
         {
            if (state[condition] === undefined)
            {
               if (seen[condition] == null)
               {
                  seen[condition] = true;
                  log("HUD rules: no condition \"" + condition + "\" (" + name + "), counted as holding");
               }
               holds = true;
            }
            else if (state[condition] == true)
            {
               holds = true;
            }
         }
         if (holds)
         {
            held[name] = now;
            return true;
         }
         return held[name] != null && now - int(held[name]) < Number(state.linger) * 1000;
      }
   }
}
