package
{
   import crui.Text;
   import flash.display.Shape;
   import flash.display.Sprite;
   import flash.text.TextField;
   import flash.text.TextFieldAutoSize;

   // The FNV preset's weapon condition bar: "CND" (translated) and a meter of the equipped weapon's condition, in
   // the HUD's bold font and color, as FNV draws it on the ammo row of its AP block (drawn by our own code: FNV's art
   // can't ship). It sits in the HUD group RightMeters_mc, so it hides and fades with it; FnvPreset adds it, places
   // it and feeds it. It always draws its full size (empty while no weapon wears), so its bounds can be measured
   // and its box placed in the edit menu while it is hidden.
   // See docs/swf_framework.md#fnv-widgets
   public class ConditionBar extends Sprite
   {
      // Its name in its group, and among the places and in the edit menu ("part:" + it).
      public static const NAME:String = "CRUI_WeaponCondition_mc";
      // The label's size (as StatsLine's), the meter's size and the gap between them (px of the group), and the
      // alpha of the meter's empty part.
      private static const SIZE:int = 20;
      private static const WIDE:Number = 96;
      private static const HIGH:Number = 10;
      private static const GAP:Number = 8;
      private static const EMPTY:Number = 0.3;
      private var label:TextField;
      private var meter:Shape = new Shape();
      private var drawn:Number = NaN;
      private var drawnColor:int = -1;

      public function ConditionBar()
      {
         super();
         name = NAME;
         mouseEnabled = false;
         mouseChildren = false;
         label = Text.field(SIZE, null, Text.BOLD);
         label.autoSize = TextFieldAutoSize.LEFT;
         Text.show(label, "$CRUI_CND");
         addChild(label);
         addChild(meter);
         update(-1, 0xFFFFFF, false);
      }

      // Show `health` (0 to 1; below 0: no weapon that wears, hidden) in `color`, while `shown`.
      public function update(health:Number, color:uint, shown:Boolean) : void
      {
         visible = shown && health >= 0;
         var full:Number = Math.max(0, Math.min(1, health));
         if (full == drawn && int(color) == drawnColor)
         {
            return;
         }
         drawn = full;
         drawnColor = int(color);
         label.textColor = color;
         meter.x = label.width + GAP;
         meter.y = (label.height - HIGH) / 2;
         meter.graphics.clear();
         meter.graphics.beginFill(color, EMPTY);
         meter.graphics.drawRect(WIDE * full, 0, WIDE * (1 - full), HIGH);
         meter.graphics.endFill();
         meter.graphics.beginFill(color, 1);
         meter.graphics.drawRect(0, 0, WIDE * full, HIGH);
         meter.graphics.endFill();
      }
   }
}
