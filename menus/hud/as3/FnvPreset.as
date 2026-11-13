package
{
   import flash.display.DisplayObject;
   import flash.display.DisplayObjectContainer;
   import flash.display.Stage;
   import flash.geom.Point;
   import flash.geom.Rectangle;

   // The FNV layout preset ([Hud] sPreset=FNV in CleanroomUI.ini): the game's own HUD parts arranged as Fallout: New
   // Vegas arranges its HUD. FNV's menu files (hud_main_menu.xml) name its parts but set no positions (its engine
   // places them from the safe area), so the preset is rules, worked out each time from the parts' own bounds where
   // the game put them: right at any screen shape and HUD size. No part of the game's leaves its group (the engine
   // may reach a part by its path); each is moved within it.
   // - Bottom left: the compass under the HP bar, left edges together, at the HP bar's bottom; the HP bar, the rads
   //   meter and the location text go up above it (FNV's HP bracket holds the compass).
   // - Bottom right: the ammo count under the AP bar, right edges together, at the AP bar's bottom; the AP bar goes
   //   up above it (FNV: AP, then weapon condition and ammo). Our weapon condition bar (ConditionBar, the one part
   //   FNV has there and FO4 has not) goes on the ammo count's row, at the AP bar's left edge.
   // - Bottom center: the enemy health meter just above the critical meter (FNV shows it low, under the reticle).
   // - Center: the sneak meter just above the screen's middle (FNV's [HIDDEN]/[CAUTION] sits over the reticle).
   // A part with no bounds (nothing drawn) is left where it is. The player's places go on top (PartLayout).
   // See docs/swf_framework.md#fnv-preset and docs/swf_framework.md#fnv-widgets
   public class FnvPreset
   {
      // The preset's name in the INI.
      public static const NAME:String = "FNV";
      // Our own widgets, added to the HUD's groups and laid out and placed as its parts are: name -> group.
      public static const WIDGETS:Object = {CRUI_WeaponCondition_mc: "RightMeters_mc"};
      // The gap between two parts (px of the stage), and how far above the middle the sneak meter's bottom sits
      // (a fraction of the stage's height).
      private static const GAP:Number = 4;
      private static const SNEAK_RISE:Number = 0.06;
      // How often the weapon's condition and the HUD color are read again (frames).
      private static const READ_FRAMES:int = 6;
      private var condition:ConditionBar;
      private var health:Number = -1;
      private var color:uint = 0xFFFFFF;
      private var frames:int = 0;

      // Our widgets added to their groups under `hud` (the HUD's root) when not there (the HUD made again).
      public function add(hud:Object) : void
      {
         var group:DisplayObjectContainer = groupOf(hud, WIDGETS[ConditionBar.NAME]);
         if (group != null && group.getChildByName(ConditionBar.NAME) == null)
         {
            condition = new ConditionBar();
            group.addChild(condition);
         }
      }

      // Each frame: the condition bar fed (the weapon's condition and the HUD color read every few frames) and
      // shown while the AP bar is.
      public function draw(hud:Object, dll:Object) : void
      {
         if (condition == null)
         {
            return;
         }
         if (frames++ % READ_FRAMES == 0)
         {
            health = dll.HudWeaponCondition != null ? Number(dll.HudWeaponCondition()) : -1;
            color = dll.HudColor != null ? uint(dll.HudColor()) : 0xFFFFFF;
         }
         var group:DisplayObjectContainer = groupOf(hud, WIDGETS[ConditionBar.NAME]);
         var ap:DisplayObject = group != null ? group.getChildByName("ActionPointMeter_mc") : null;
         condition.update(health, color, ap == null || ap.visible);
         condition.alpha = ap != null ? ap.alpha : 1;
      }

      // Arrange `parts` (name -> {view, now {x, y, scaleX, scaleY}}, each already at `now`) on `stage`; `now` and
      // the views are moved together.
      public function arrange(parts:Object, stage:Stage) : void
      {
         var hp:Rectangle = bounds(parts, "HPMeter_mc", stage);
         var compass:Rectangle = bounds(parts, "CompassWidget_mc", stage);
         if (hp != null && compass != null)
         {
            moveBy(parts, "CompassWidget_mc", hp.left - compass.left, hp.bottom - compass.bottom);
            for each (var onHp:String in ["HPMeter_mc", "RadsMeter_mc", "LocationText_mc"])
            {
               moveBy(parts, onHp, 0, -(compass.height + GAP));
            }
         }
         var ap:Rectangle = bounds(parts, "ActionPointMeter_mc", stage);
         var ammo:Rectangle = bounds(parts, "AmmoCount_mc", stage);
         if (ap != null && ammo != null)
         {
            moveBy(parts, "AmmoCount_mc", ap.right - ammo.right, ap.bottom - ammo.bottom);
            moveBy(parts, "ActionPointMeter_mc", 0, -(ammo.height + GAP));
         }
         ap = bounds(parts, "ActionPointMeter_mc", stage);
         ammo = bounds(parts, "AmmoCount_mc", stage);
         var cnd:Rectangle = bounds(parts, ConditionBar.NAME, stage);
         if (ap != null && cnd != null)
         {
            // On the ammo count's row (centered on it), or just under the AP bar without one.
            var row:Number = ammo != null ? ammo.top + ammo.height / 2 : ap.bottom + GAP + cnd.height / 2;
            moveBy(parts, ConditionBar.NAME, ap.left - cnd.left, row - (cnd.top + cnd.height / 2));
         }
         var enemy:Rectangle = bounds(parts, "EnemyHealthMeter_mc", stage);
         var crit:Rectangle = bounds(parts, "CritMeter_mc", stage);
         if (enemy != null && crit != null)
         {
            moveBy(parts, "EnemyHealthMeter_mc", (crit.left + crit.width / 2) - (enemy.left + enemy.width / 2),
                   crit.top - GAP - enemy.bottom);
         }
         var sneak:Rectangle = bounds(parts, "StealthMeter_mc", stage);
         if (sneak != null)
         {
            moveBy(parts, "StealthMeter_mc", stage.stageWidth / 2 - (sneak.left + sneak.width / 2),
                   stage.stageHeight * (0.5 - SNEAK_RISE) - sneak.bottom);
         }
      }

      // HUD group `name` under `hud`, or null.
      private static function groupOf(hud:Object, name:String) : DisplayObjectContainer
      {
         try
         {
            return hud[name] as DisplayObjectContainer;
         }
         catch (error:Error)
         {
         }
         return null;
      }

      // Part `name`'s bounds on the stage, or null when it is not found or draws nothing.
      private static function bounds(parts:Object, name:String, stage:Stage) : Rectangle
      {
         var part:Object = parts[name];
         if (part == null || part.view == null)
         {
            return null;
         }
         var box:Rectangle = DisplayObject(part.view).getBounds(stage);
         return box.width > 0 && box.height > 0 ? box : null;
      }

      // Move part `name` by dx, dy (px of the stage), in its own group.
      private static function moveBy(parts:Object, name:String, dx:Number, dy:Number) : void
      {
         var part:Object = parts[name];
         if (part == null || part.view == null)
         {
            return;
         }
         var view:DisplayObject = part.view;
         var from:Point = view.parent.globalToLocal(new Point(0, 0));
         var to:Point = view.parent.globalToLocal(new Point(dx, dy));
         part.now.x += to.x - from.x;
         part.now.y += to.y - from.y;
         view.x = part.now.x;
         view.y = part.now.y;
      }
   }
}
