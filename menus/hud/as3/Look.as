package
{
   import flash.display.DisplayObject;
   import flash.display.DisplayObjectContainer;
   import flash.display.Shape;
   import flash.geom.ColorTransform;
   import flash.geom.Rectangle;
   import flash.utils.Dictionary;

   // How the player's look (hidden, a color; set in the edit menu) goes on the game's own HUD parts, each frame.
   // The engine shows, hides and fades the parts itself (it sets their visible and alpha from C++; HUDMenu.as sets
   // none), so neither is ours to set: we would fight it, and lose what it last wanted.
   // - Hidden: a mask that covers nothing (a tenth of a pixel at the part's own top-left, inside the part so its
   //   bounds don't change), which the game never sets. The mask hides what Flash draws; the shaded background the
   //   engine draws from C++ behind some parts (the compass's dark band) is switched off by CleanroomUI.dll for the
   //   parts hidden() lists (HudBackgrounds). Taking the mask off shows the part as the game has it now.
   // - A color: the color transform's multipliers (alpha and offsets left the game's), set again whenever they are
   //   not ours, so the game writing its own over them is undone the next frame; the game's own are kept from its
   //   last write and put back when the color is taken off.
   // A part never given a look is never touched.
   // See docs/swf_framework.md#hud-look
   public class Look
   {
      // The mask's side (px of the part), and how near two multipliers are to count as the same (Scaleform keeps a
      // transform at about 1/256).
      private static const MASK_SIDE:Number = 0.1;
      private static const SAME:Number = 0.005;
      // View -> {mask (our Shape while hidden, or null), own ([r, g, b] the game's multipliers), wrote ([r, g, b]
      // ours as last read back, or null)}.
      private var states:Dictionary = new Dictionary(true);

      // The views hidden by us now.
      public function hidden() : Array
      {
         var out:Array = [];
         for (var view:Object in states)
         {
            if (states[view].mask != null)
            {
               out.push(view);
            }
         }
         return out;
      }

      // `view` hidden or shown, in `color` (0xRRGGBB) or, below 0, its own.
      public function apply(view:DisplayObject, hidden:Boolean, color:Number) : void
      {
         var state:Object = states[view];
         if (state == null)
         {
            if (!hidden && !(color >= 0))
            {
               return;
            }
            state = {mask: null, own: null, wrote: null};
            states[view] = state;
         }
         hide(view, state, hidden);
         tint(view, state, color);
      }

      private static function hide(view:DisplayObject, state:Object, hidden:Boolean) : void
      {
         var holder:DisplayObjectContainer = view as DisplayObjectContainer;
         if (hidden && holder != null)
         {
            if (state.mask == null)
            {
               var corner:Rectangle = view.getBounds(view);
               var mask:Shape = new Shape();
               mask.name = "CRUI_HideMask";
               mask.graphics.beginFill(0);
               mask.graphics.drawRect(corner.x, corner.y, MASK_SIDE, MASK_SIDE);
               mask.graphics.endFill();
               holder.addChild(mask);
               state.mask = mask;
            }
            if (view.mask != state.mask)
            {
               view.mask = state.mask;
            }
         }
         else if (state.mask != null)
         {
            if (view.mask == state.mask)
            {
               view.mask = null;
            }
            if (state.mask.parent != null)
            {
               state.mask.parent.removeChild(state.mask);
            }
            state.mask = null;
         }
      }

      private static function tint(view:DisplayObject, state:Object, color:Number) : void
      {
         var now:ColorTransform = view.transform.colorTransform;
         var have:Array = [now.redMultiplier, now.greenMultiplier, now.blueMultiplier];
         if (state.wrote == null || !same(have, state.wrote))
         {
            // First seen, or the game wrote its own since: keep it for when our color comes off.
            state.own = have;
         }
         var want:Array = color >= 0 ? multipliers(uint(color)) : (state.wrote != null ? state.own : null);
         if (want != null && !same(have, want))
         {
            now.redMultiplier = want[0];
            now.greenMultiplier = want[1];
            now.blueMultiplier = want[2];
            view.transform.colorTransform = now;
         }
         if (color >= 0)
         {
            var back:ColorTransform = view.transform.colorTransform;
            state.wrote = [back.redMultiplier, back.greenMultiplier, back.blueMultiplier];
         }
         else
         {
            state.wrote = null;
         }
      }

      // `color`'s multipliers, [r, g, b] from 0 to 1.
      public static function multipliers(color:uint) : Array
      {
         return [((color >> 16) & 0xFF) / 255, ((color >> 8) & 0xFF) / 255, (color & 0xFF) / 255];
      }

      private static function same(a:Array, b:Array) : Boolean
      {
         for (var i:int = 0; i < 3; i++)
         {
            if (Math.abs(Number(a[i]) - Number(b[i])) > SAME)
            {
               return false;
            }
         }
         return true;
      }
   }
}
