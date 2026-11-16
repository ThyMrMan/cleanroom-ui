package
{
   import flash.display.DisplayObject;
   import flash.geom.Point;
   import flash.geom.Rectangle;

   // A HUD part's or widget's outline for the edit menu: every bounds it has had, in its own space, joined. Parts that
   // animate (the Vault Boy, the hit direction arrows) change their bounds frame by frame, and a box measured afresh
   // at each report grew, shrank and slid (played 2026-11-15); the joined outline settles once its animation has run.
   // Kept in the clip's own space, so moving or scaling it never changes the outline, only where it lands.
   // See docs/swf_framework.md#hud-edit
   public class Outline
   {
      private var joined:Rectangle = null;

      // `view`'s outline on `stage` where it is now: its bounds this frame joined with every earlier one.
      public function measure(view:DisplayObject, stage:DisplayObject) : Rectangle
      {
         var now:Rectangle = view.getBounds(view);
         if (now.width > 0 || now.height > 0)
         {
            joined = joined == null ? now : joined.union(now);
         }
         if (joined == null)
         {
            return view.getBounds(stage);
         }
         var corners:Array = [new Point(joined.left, joined.top), new Point(joined.right, joined.top),
                              new Point(joined.left, joined.bottom), new Point(joined.right, joined.bottom)];
         var left:Number = Infinity, top:Number = Infinity, right:Number = -Infinity, bottom:Number = -Infinity;
         for each (var corner:Point in corners)
         {
            var at:Point = stage.globalToLocal(view.localToGlobal(corner));
            left = Math.min(left, at.x);
            top = Math.min(top, at.y);
            right = Math.max(right, at.x);
            bottom = Math.max(bottom, at.y);
         }
         return new Rectangle(left, top, right - left, bottom - top);
      }
   }
}
