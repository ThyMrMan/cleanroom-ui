package
{
   import flash.display.DisplayObject;
   import flash.display.DisplayObjectContainer;
   import flash.text.TextField;
   import flash.utils.getTimer;

   // Whether a box (a widget of ours, or one of the game's HUD parts) is shown now by its rule: the conditions it
   // shows under, comma separated, from its place (`show`, the player's or the preset's; CleanroomUI.dll's
   // ShowRule). With no rule it is always shown. A box shows while any of its conditions holds and for the linger
   // after the last one ends, so it doesn't flicker. A condition nobody knows counts as holding (logged once), so a
   // typo never hides a box for good; so does every rule without the DLL.
   // - combat, drawn, vats, peek, aiming: from the DLL (HudState), once a frame.
   // - sneaking: the game's sneak meter (StealthMeter_mc) is on screen: it and every clip above it visible and not
   //   faded out. The engine shows it only while the player sneaks; our own hiding (a mask) doesn't change that.
   // - changed: the box's value changed (a meter's, from HudValues, a few times a second: HP, AP, rads, the CND
   //   bar's condition), or, for a part with no value (the ammo count, the explosives count, ...), the text it shows.
   // - low (low:F for a threshold F from 0 to 1): the box's value is under F (HP, AP, the CND bar: 0.5 by default),
   //   or the ammo count's magazine is at or under F (0.25) of the fullest it has been with this weapon.
   // - moved: the location's name or the quest updates changed (for the compass).
   // Our widgets have no value: changed and low never hold for them.
   // See docs/swf_framework.md#hud-conditions
   public class ShowRules
   {
      // Each part's value in HudValues (the rest are read as their text).
      private static const VALUES:Object = {
         "part:HPMeter_mc": "health", "part:ActionPointMeter_mc": "ap", "part:RadsMeter_mc": "rads",
         "part:CRUI_WeaponCondition_mc": "condition"};
      // The parts the host hands update(): those whose text `moved` watches, then the sneak meter.
      public static const WATCHED:Array = ["LocationText_mc", "QuestUpdates_mc", "StealthMeter_mc"];
      private static const MOVED:int = 2;
      // How faded a clip may be and still count as on screen.
      private static const SHOWN_ALPHA:Number = 0.05;
      // The ammo count's part (low is its magazine).
      private static const AMMO:String = "part:AmmoCount_mc";
      // The default thresholds of low, how much a value must move to count as changed, how often the values are
      // read (frames), and how deep a part's clips are searched for its text.
      private static const LOW:Number = 0.5;
      private static const LOW_AMMO:Number = 0.25;
      private static const MOVE:Number = 0.005;
      private static const VALUE_FRAMES:int = 6;
      private static const TEXT_DEPTH:int = 5;
      private var state:Object;
      private var values:Object;
      private var frames:int = 0;
      // Box name -> when its rule last held, when it last changed (ms, getTimer); its last reading (value or text).
      private var held:Object = {};
      private var changedAt:Object = {};
      private var readings:Object = {};
      // The ammo count: the fullest magazine seen, and the weapon it was seen with.
      private var fullest:Number = 0;
      private var fullestWeapon:Number = -1;
      // The frame `moved` last held on, and whether the sneak meter is on screen.
      private var movedFrame:int = -1;
      private var sneaking:Boolean = false;
      private var log:Function;
      private var seen:Object = {};

      // `log` takes a line.
      public function ShowRules(log:Function)
      {
         this.log = log;
      }

      // The conditions now, from `dll` (HudState each frame, HudValues every few), and `moved` and `sneaking` from
      // the parts `watched` (WATCHED's views, null where not found).
      public function update(dll:Object, watched:Array) : void
      {
         state = dll != null && dll.HudState != null ? dll.HudState() : null;
         if (frames++ % VALUE_FRAMES == 0 && dll != null && dll.HudValues != null)
         {
            values = dll.HudValues();
         }
         for (var i:int = 0; i < MOVED; i++)
         {
            // A new name or objective, not one being cleared away.
            var text:String = watched != null ? textOf(watched[i]) : null;
            if (note("moved:" + WATCHED[i], text, getTimer()) && text != null && text.replace(/[|\s]/g, "") != "")
            {
               movedFrame = frames;
            }
         }
         sneaking = watched != null && onScreen(watched[MOVED]);
      }

      // Whether box `name` under rule `rule` ("" or null: always) is shown now; `view` is the part (null for a
      // widget of ours).
      public function shows(name:String, rule:String, view:DisplayObject = null) : Boolean
      {
         if (rule == null || rule == "" || state == null)
         {
            return true;
         }
         var now:int = getTimer();
         var holds:Boolean = false;
         var reading:Object = null;
         for each (var condition:String in rule.split(","))
         {
            var word:String = condition.split(":")[0];
            var given:Number = condition.indexOf(":") > 0 ? Number(condition.split(":")[1]) : NaN;
            if (word == "changed" || word == "low")
            {
               if (reading == null)
               {
                  reading = {value: read(name, view)};
                  if (note(name, reading.value, now))
                  {
                     changedAt[name] = now;
                  }
               }
               holds = word == "changed" ? holds || changedAt[name] == now
                                         : holds || low(name, reading.value, given);
            }
            else if (word == "moved")
            {
               holds = holds || movedFrame == frames;
            }
            else if (word == "sneaking")
            {
               holds = holds || sneaking;
            }
            else if (state[word] === undefined)
            {
               if (seen[word] == null)
               {
                  seen[word] = true;
                  log("HUD rules: no condition \"" + word + "\" (" + name + "), counted as holding");
               }
               holds = true;
            }
            else if (state[word] == true)
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

      // Box `name`'s reading: its value (a number; null while none is read yet) or, with no value, its text (null for
      // a widget of ours).
      private function read(name:String, view:DisplayObject) : Object
      {
         if (VALUES[name] != null)
         {
            return values != null && values[VALUES[name]] != null ? Number(values[VALUES[name]]) : null;
         }
         return view != null ? textOf(view) : null;
      }

      // Remember `reading` as `key`'s; whether it changed from the last one (the first is no change).
      private function note(key:String, reading:Object, now:int) : Boolean
      {
         var last:Object = readings[key];
         readings[key] = reading;
         if (last == null || reading == null)
         {
            return false;
         }
         if (reading is Number && last is Number)
         {
            return Math.abs(Number(reading) - Number(last)) > MOVE;
         }
         return String(reading) != String(last);
      }

      // Whether box `name`'s reading is low: a value under `given` (else LOW), or the ammo count's magazine at or
      // under `given` (else LOW_AMMO) of the fullest seen with this weapon.
      private function low(name:String, reading:Object, given:Number) : Boolean
      {
         if (reading is Number)
         {
            return Number(reading) >= 0 && Number(reading) < (isNaN(given) ? LOW : given);
         }
         if (name != AMMO || reading == null)
         {
            return false;
         }
         var found:Array = String(reading).match(/\d+/);
         if (found == null)
         {
            return false;
         }
         var magazine:Number = Number(found[0]);
         var weapon:Number = values != null ? Number(values.weapon) : 0;
         if (weapon != fullestWeapon)
         {
            fullestWeapon = weapon;
            fullest = 0;
         }
         fullest = Math.max(fullest, magazine);
         return fullest > 0 && magazine <= Math.floor(fullest * (isNaN(given) ? LOW_AMMO : given));
      }

      // Whether `view` is on screen: it and every clip above it visible, and not faded out.
      private static function onScreen(view:DisplayObject) : Boolean
      {
         if (view == null || view.stage == null)
         {
            return false;
         }
         var alpha:Number = 1;
         for (var at:DisplayObject = view; at != null && at != at.stage; at = at.parent)
         {
            if (!at.visible)
            {
               return false;
            }
            alpha *= at.alpha;
         }
         return alpha > SHOWN_ALPHA;
      }

      // The text `view` shows: its text fields' text, in order, joined (null for no view).
      private static function textOf(view:DisplayObject) : String
      {
         if (view == null)
         {
            return null;
         }
         var out:Array = [];
         collect(view, out, TEXT_DEPTH);
         return out.join("|");
      }

      private static function collect(view:DisplayObject, out:Array, depth:int) : void
      {
         var field:TextField = view as TextField;
         if (field != null)
         {
            if (field.visible)
            {
               out.push(field.text);
            }
            return;
         }
         var holder:DisplayObjectContainer = view as DisplayObjectContainer;
         if (holder == null || depth <= 0)
         {
            return;
         }
         for (var i:int = 0; i < holder.numChildren; i++)
         {
            collect(holder.getChildAt(i), out, depth - 1);
         }
      }
   }
}
