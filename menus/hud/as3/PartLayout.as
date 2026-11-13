package
{
   import flash.display.DisplayObject;
   import flash.geom.ColorTransform;

   // The game's own HUD parts laid out from DEF_HUD.xml, as CleanroomUI.dll reads it (HudPartLayout, parts.cpp): each
   // part is found by its clip name in its HUD group (parname: one of the seven groups the game pins to the safe area,
   // so one layout serves every screen shape) and given the file's position, scale and rotation, and its color
   // (a tint over the HUD color). The values are DEF's own, absolute in the group, so with DEF_HUD's own HUDMenu.swf
   // installed both set the same values. Only what the file changes from the game's defaults is set: a position
   // only when it differs from defx, defy; a scale other than 1; a rotation other than the part's own; a color other
   // than white. Everything else stays the game's. apply() is called again every few frames, for parts the game
   // moved or made again (power armor, a HUD mode change); a value already right is left alone.
   // Not applied yet: flags (which of a part's pieces are shown; DEF's meaning of each bit is not known), color2,
   // SafeX, SafeY and globalscaleX, globalscaleY; each is logged once when the file sets it.
   // See docs/swf_framework.md#hud-layout
   public class PartLayout
   {
      // The HUD's groups DEF's parname names; anything else is not a HUD part.
      private static const GROUPS:Object = {HUDNotificationsGroup_mc: true, TopCenterGroup_mc: true,
                                            TopRightGroup_mc: true, CenterGroup_mc: true, LeftMeters_mc: true,
                                            BottomCenterGroup_mc: true, RightMeters_mc: true};
      // A part's own rotation, seen the first time it was found (by name), to tell a rotation the file changes.
      private var ownRotation:Object = {};
      private var parts:Array = [];
      private var log:Function;
      private var seen:Object = {};

      // `layout` as HudPartLayout gives it ({settings, parts}); `log` takes a line.
      public function PartLayout(layout:Object, log:Function)
      {
         this.log = log;
         if (layout == null)
         {
            return;
         }
         parts = layout.parts as Array || [];
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
         log("HUD layout: " + parts.length + " parts from DEF_HUD.xml");
      }

      // Lay out every part found under `hud` (the HUD's root).
      public function apply(hud:Object) : void
      {
         for each (var entry:Object in parts)
         {
            var part:DisplayObject = find(hud, String(entry.parname), String(entry.name));
            if (part != null)
            {
               place(part, entry);
            }
         }
      }

      // The part `name` in HUD group `group` under `hud`, or null.
      private function find(hud:Object, group:String, name:String) : DisplayObject
      {
         if (GROUPS[group] == null)
         {
            once(name + ": " + group + " is not one of the HUD's groups, left alone");
            return null;
         }
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

      private function place(part:DisplayObject, entry:Object) : void
      {
         var name:String = String(entry.name);
         if (ownRotation[name] == null)
         {
            ownRotation[name] = part.rotation;
         }
         if (isNumber(entry.x) && isNumber(entry.defx) && Number(entry.x) != Number(entry.defx))
         {
            set(part, "x", Number(entry.x));
         }
         if (isNumber(entry.y) && isNumber(entry.defy) && Number(entry.y) != Number(entry.defy))
         {
            set(part, "y", Number(entry.y));
         }
         if (isNumber(entry.scaleX) && Number(entry.scaleX) != 1)
         {
            set(part, "scaleX", Number(entry.scaleX));
         }
         if (isNumber(entry.scaleY) && Number(entry.scaleY) != 1)
         {
            set(part, "scaleY", Number(entry.scaleY));
         }
         if (isNumber(entry.rotation) && !sameAngle(Number(entry.rotation), Number(ownRotation[name])))
         {
            if (!sameAngle(part.rotation, Number(entry.rotation)))
            {
               part.rotation = Number(entry.rotation);
            }
         }
         if (isNumber(entry.color) && uint(entry.color) != 0xFFFFFF)
         {
            tint(part, uint(entry.color));
         }
         if (isNumber(entry.color2) && uint(entry.color2) != 0xFFFFFF)
         {
            once(name + ": color2 not applied yet");
         }
         if (isNumber(entry.flags) && uint(entry.flags) != 255)
         {
            once(name + ": flags " + entry.flags + " (hidden pieces) not applied yet");
         }
      }

      // Set `part`'s `property` to `value` unless it is that already.
      private static function set(part:DisplayObject, property:String, value:Number) : void
      {
         if (Math.abs(Number(part[property]) - value) > 0.001)
         {
            part[property] = value;
         }
      }

      // Tint `part` by `color` (multiplied over what is drawn, the HUD color included), keeping its alpha.
      private static function tint(part:DisplayObject, color:uint) : void
      {
         var red:Number = ((color >> 16) & 0xFF) / 255, green:Number = ((color >> 8) & 0xFF) / 255,
             blue:Number = (color & 0xFF) / 255;
         var now:ColorTransform = part.transform.colorTransform;
         if (now.redMultiplier != red || now.greenMultiplier != green || now.blueMultiplier != blue)
         {
            part.transform.colorTransform = new ColorTransform(red, green, blue, now.alphaMultiplier, 0, 0, 0,
                                                               now.alphaOffset);
         }
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
