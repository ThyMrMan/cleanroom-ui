package
{
   import flash.display.DisplayObject;
   import flash.display.Stage;
   import flash.geom.Point;
   import flash.geom.Rectangle;

   // The game's own HUD parts: laid out from DEF_HUD.xml, as CleanroomUI.dll reads it (HudPartLayout, parts.cpp), or
   // by a preset ([Hud] sPreset in CleanroomUI.ini; FnvPreset), then moved and scaled by the place the player gave each
   // in our edit menu (HudPlaceOf, the same places our widgets
   // have, kept under "part:" and the clip's name).
   //
   // Each part is found by its clip name in its HUD group (one of the seven groups the game pins to the safe area, so
   // one layout serves every screen shape). Its base is where the game put it, with DEF_HUD.xml's position and scale
   // over it where the file changes them from the game's defaults (a position other than defx, defy; a scale other
   // than 1); DEF's values are absolute in the group, so with DEF_HUD's own HUDMenu.swf installed both set the same.
   // The base is reported to CleanroomUI.dll for the edit menu (HudReport, as a widget's is), then the player's place
   // moves and scales the part from it. The file's rotation is set when it differs from the part's own. The part's
   // look (Look) is put on each frame: hidden as the player set it, and the player's color, else the file's (a color
   // other than white), else its own. place() runs again every few frames and whenever a place changes; a part
   // the game moved itself since (power armor, a HUD mode change) takes that as its new base, and a value already
   // right is left alone.
   // Not applied yet: flags (which of a part's pieces are shown; DEF's meaning of each bit is not known), color2,
   // SafeX, SafeY and globalscaleX, globalscaleY; each is logged once when the file sets it.
   // See docs/swf_framework.md#hud-layout and docs/swf_framework.md#hud-edit
   public class PartLayout
   {
      // The prefix of a part's name among the places and in the edit menu.
      public static const PREFIX:String = "part:";
      // The vanilla HUD's parts by group: those DEF_HUD lays out, every one a clip of that name in HUDMenu.swf.
      private static const GROUPS:Object = {
         HUDNotificationsGroup_mc: ["PromptMessageHolder_mc", "TutorialText_mc", "QuestUpdates_mc", "XPMeter_mc"],
         TopCenterGroup_mc: ["EnemyHealthMeter_mc", "StealthMeter_mc"],
         TopRightGroup_mc: ["VaultBoyCondition_mc"],
         CenterGroup_mc: ["HUDCrosshair_mc", "HitIndicator_mc", "QuickContainerWidget_mc",
                          "DirectionalHitIndicatorBase_mc", "ExplosiveIndicatorBase_mc", "RolloverWidget_mc"],
         LeftMeters_mc: ["LocationText_mc", "RadsMeter_mc", "HPMeter_mc"],
         BottomCenterGroup_mc: ["PerkVaultBoy_mc", "SubtitleText_mc", "CritMeter_mc", "CompassWidget_mc"],
         RightMeters_mc: ["PowerArmorLowBatteryWarning_mc", "FlashLightWidget_mc", "ExplosiveAmmoCount_mc",
                          "AmmoCount_mc", "ActionPointMeter_mc", "FatigueWarning_mc", "HUDActiveEffectsWidget_mc"]};
      // Part name (the vanilla parts, and with the FNV preset its own widgets) -> {group, name, entry (DEF_HUD.xml's,
      // or null), base {x, y, scaleX, scaleY}, set (what it read back after we last placed it, or null), rotation
      // (its own, as first found), view (found by the last place(), or null), hidden, color (its look, from its place
      // and the file), show (its rule: the conditions it shows under, "" always), outline (Outline: the bounds it
      // reports)}.
      private var parts:Object = {};
      private var log:Function;
      private var seen:Object = {};
      // The preset to arrange the parts by ("" none; FnvPreset.NAME), and the FNV preset with its own widgets.
      private var preset:String = "";
      private var fnv:FnvPreset;
      private var looks:Look = new Look();

      // `layout` as HudPartLayout gives it ({preset, settings, parts}, parts empty with no file; or null with no
      // DLL); `log` takes a line.
      public function PartLayout(layout:Object, log:Function)
      {
         this.log = log;
         for (var group:String in GROUPS)
         {
            for each (var name:String in GROUPS[group])
            {
               parts[name] = {group: group, name: name, entry: null};
            }
         }
         if (layout == null)
         {
            return;
         }
         preset = String(layout.preset || "").toUpperCase();
         if (preset != "" && preset != FnvPreset.NAME)
         {
            once("no preset " + layout.preset + ", none used");
            preset = "";
         }
         if (preset == FnvPreset.NAME)
         {
            fnv = new FnvPreset();
            for (var widget:String in FnvPreset.WIDGETS)
            {
               parts[widget] = {group: FnvPreset.WIDGETS[widget], name: widget, entry: null};
            }
         }
         if (preset != "")
         {
            var ignored:int = (layout.parts as Array || []).length;
            log("HUD layout: the " + preset + " preset" + (ignored > 0 ? " (DEF_HUD.xml left aside)" : ""));
            return;
         }
         var count:int = 0;
         for each (var entry:Object in layout.parts as Array || [])
         {
            var known:Object = parts[String(entry.name)];
            if (known == null || known.group != String(entry.parname))
            {
               once(entry.name + " in " + entry.parname + " is not one of the HUD's parts, left alone");
               continue;
            }
            known.entry = entry;
            count++;
         }
         var settings:Object = layout.settings || {};
         for each (var setting:String in ["SafeX", "SafeY"])
         {
            if (settings[setting] != null)
            {
               once(setting + " " + settings[setting] + " not applied: the game's own safe area is kept");
            }
         }
         for each (var scale:String in ["globalscaleX", "globalscaleY"])
         {
            if (settings[scale] != null && Number(settings[scale]) != 1)
            {
               once(scale + " " + settings[scale] + " not applied yet");
            }
         }
         log("HUD layout: " + count + " parts from DEF_HUD.xml");
      }

      // Lay out and place every part found under `hud` (the HUD's root), reporting each to `dll`: each part at its
      // base with DEF_HUD.xml's changes, then the preset over all of them, then each reported and moved by its place.
      public function place(hud:Object, dll:Object, stage:Stage) : void
      {
         if (fnv != null)
         {
            fnv.add(hud);
         }
         var found:Object = {};
         for each (var part:Object in parts)
         {
            var view:DisplayObject = find(hud, part.group, part.name);
            part.view = view != null && view.parent != null ? view : null;
            if (part.view != null)
            {
               lay(part);
               found[part.name] = part;
            }
         }
         if (fnv != null && stage != null)
         {
            fnv.arrange(found, stage);
         }
         for each (part in found)
         {
            finish(part, dll, stage);
         }
      }

      // Each frame: the preset's own widgets drawn (FnvPreset.draw), then every part's look put on (again over any
      // the game wrote since): hidden by the player, or by its rule (`rules`) while none of its conditions holds.
      public function draw(hud:Object, dll:Object, rules:ShowRules) : void
      {
         if (fnv != null)
         {
            fnv.draw(hud, dll);
         }
         for each (var part:Object in parts)
         {
            if (part.view != null)
            {
               var hidden:Boolean = part.hidden == true || !rules.shows(PREFIX + part.name, part.show, part.view);
               looks.apply(part.view, hidden, part.color != null ? Number(part.color) : -1);
            }
         }
      }

      // The parts the show rules watch (ShowRules.WATCHED), as found by the last place() (null where not found).
      public function watched() : Array
      {
         var out:Array = [];
         for each (var name:String in ShowRules.WATCHED)
         {
            out.push(parts[name] != null ? parts[name].view : null);
         }
         return out;
      }

      // The part `name` in HUD group `group` under `hud`, or null.
      private function find(hud:Object, group:String, name:String) : DisplayObject
      {
         var found:DisplayObject = null;
         try
         {
            found = hud[group][name] as DisplayObject;
            if (found == null && hud[group] != null)
            {
               found = hud[group].getChildByName(name);
            }
         }
         catch (error:Error)
         {
            found = null;
         }
         if (found == null)
         {
            once("no HUD part " + group + "." + name + " (another HUD movie?), left alone");
         }
         return found;
      }

      // Part `part` at its base, with DEF_HUD.xml's position and scale over it (part.now), and the file's rotation
      // and color.
      private function lay(part:Object) : void
      {
         var view:DisplayObject = part.view;
         var set:Object = part.set;
         if (set == null || !near(view.x, set.x) || !near(view.y, set.y) || !near(view.scaleX, set.scaleX)
             || !near(view.scaleY, set.scaleY))
         {
            // First found, or the game moved it since: where it is now is its base.
            part.base = {x: view.x, y: view.y, scaleX: view.scaleX, scaleY: view.scaleY};
            if (part.rotation == null)
            {
               part.rotation = view.rotation;
            }
         }
         var entry:Object = part.entry;
         var base:Object = {x: part.base.x, y: part.base.y, scaleX: part.base.scaleX, scaleY: part.base.scaleY};
         part.now = base;
         if (entry != null)
         {
            if (isNumber(entry.x) && isNumber(entry.defx) && Number(entry.x) != Number(entry.defx))
            {
               base.x = Number(entry.x);
            }
            if (isNumber(entry.y) && isNumber(entry.defy) && Number(entry.y) != Number(entry.defy))
            {
               base.y = Number(entry.y);
            }
            if (isNumber(entry.scaleX) && Number(entry.scaleX) != 1)
            {
               base.scaleX = Number(entry.scaleX);
            }
            if (isNumber(entry.scaleY) && Number(entry.scaleY) != 1)
            {
               base.scaleY = Number(entry.scaleY);
            }
            dress(view, part, entry);
         }
         write(view, base);
      }

      // Part `part`, at part.now, reported to `dll` (origin and bounds, in fractions of the screen), then moved and
      // scaled by its place.
      private function finish(part:Object, dll:Object, stage:Stage) : void
      {
         var view:DisplayObject = part.view;
         var base:Object = part.now;
         var name:String = PREFIX + part.name;
         var at:Object = dll.HudPlaceOf != null ? dll.HudPlaceOf(name) : null;
         var target:Object = base;
         var own:Number = part.entry != null && isNumber(part.entry.color) && uint(part.entry.color) != 0xFFFFFF
                          ? Number(uint(part.entry.color)) : -1;
         part.hidden = at != null && at.hidden == true;
         part.show = at != null && at.show != null ? String(at.show) : "";
         part.color = at != null && Number(at.color) >= 0 ? Number(at.color) : own;
         if (stage != null)
         {
            var wide:Number = stage.stageWidth, high:Number = stage.stageHeight;
            var origin:Point = view.parent.localToGlobal(new Point(base.x, base.y));
            if (part.outline == null)
            {
               part.outline = new Outline();
            }
            var bounds:Rectangle = part.outline.measure(view, stage);
            dll.HudReport(name, origin.x / wide, origin.y / high, bounds.x / wide, bounds.y / high,
                          bounds.width / wide, bounds.height / high);
            if (at != null)
            {
               var moved:Point = view.parent.globalToLocal(new Point(origin.x + Number(at.dx) * wide,
                                                                     origin.y + Number(at.dy) * high));
               target = {x: moved.x, y: moved.y, scaleX: base.scaleX * Number(at.scale),
                         scaleY: base.scaleY * Number(at.scale)};
            }
         }
         write(view, target);
         // What the clip holds now: Flash keeps x and y in twentieths of a pixel, so it reads back a little off what
         // was written, and comparing with what was written took every part for one the game moved, its moved spot
         // for its new base, and the place added again on top (played 2026-11-15: a dragged part ran off the screen).
         part.set = {x: view.x, y: view.y, scaleX: view.scaleX, scaleY: view.scaleY};
      }

      // The file's rotation on `view` (its color is part of the look: draw()), and a note for what is not applied
      // yet.
      private function dress(view:DisplayObject, part:Object, entry:Object) : void
      {
         if (isNumber(entry.rotation) && !sameAngle(Number(entry.rotation), Number(part.rotation))
             && !sameAngle(view.rotation, Number(entry.rotation)))
         {
            view.rotation = Number(entry.rotation);
         }
         if (isNumber(entry.color2) && uint(entry.color2) != 0xFFFFFF)
         {
            once(part.name + ": color2 not applied yet");
         }
         if (isNumber(entry.flags) && uint(entry.flags) != 255)
         {
            once(part.name + ": flags " + entry.flags + " (hidden pieces) not applied yet");
         }
      }

      // `view` at `at` ({x, y, scaleX, scaleY}), each value only when it is not that already.
      private static function write(view:DisplayObject, at:Object) : void
      {
         for each (var property:String in ["x", "y", "scaleX", "scaleY"])
         {
            if (!near(Number(view[property]), Number(at[property])))
            {
               view[property] = Number(at[property]);
            }
         }
      }

      private static function near(a:Number, b:Number) : Boolean
      {
         return Math.abs(a - b) <= 0.001;
      }

      private static function sameAngle(a:Number, b:Number) : Boolean
      {
         var turn:Number = ((a - b) % 360 + 360) % 360;
         return turn < 0.01 || turn > 359.99;
      }

      private static function isNumber(value:Object) : Boolean
      {
         return value is Number && !isNaN(Number(value));
      }

      private function once(what:String) : void
      {
         if (seen[what] == null)
         {
            seen[what] = true;
            log("HUD layout: " + what);
         }
      }
   }
}
