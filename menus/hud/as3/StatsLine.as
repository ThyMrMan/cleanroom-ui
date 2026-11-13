package
{
   import crui.Text;
   import flash.display.Sprite;
   import flash.text.TextField;
   import flash.text.TextFieldAutoSize;

   // One stats line on the HUD (CRUI_Hud.AddStats): each label (a "$KEY" is translated) with its actor value's whole
   // number, left to right in the HUD's bold font. A value below the first step is dim, from the second it takes the
   // HUD's warning color, from the third it flashes; a step not given never applies. The line shows while
   // CleanroomUI.dll says it is shown.
   // See docs/swf_framework.md#hud
   public class StatsLine extends Sprite
   {
      private static const SIZE:int = 20;
      private static const GAP:Number = 24;
      private static const DIM:Number = 0.5;
      private var fields:Array = [];
      // Each field's label as given, and as shown (translated once).
      private var given:Array = [];
      private var labels:Array = [];

      public function StatsLine()
      {
         super();
         mouseEnabled = false;
         mouseChildren = false;
      }

      // Draw `data` (HudStats: {shown, rows: [{text, value}], steps}, or null) in colors `normal` and `warning`,
      // flashing values hidden while `flashOff`.
      public function update(data:Object, normal:uint, warning:uint, flashOff:Boolean) : void
      {
         visible = data != null && data.shown;
         if (!visible)
         {
            return;
         }
         var rows:Array = data.rows as Array;
         var steps:Array = data.steps as Array;
         var x:Number = 0;
         var i:int = 0;
         for (; i < rows.length; i++)
         {
            var field:TextField = fieldAt(i, String(rows[i].text));
            var value:Number = Number(rows[i].value);
            var text:String = labels[i] + " " + Math.floor(value);
            if (field.text != text)
            {
               Text.show(field, text);
            }
            field.textColor = value >= step(steps, 1) ? warning : normal;
            field.alpha = value < step(steps, 0, Number.NEGATIVE_INFINITY) ? DIM : 1;
            field.visible = !(flashOff && value >= step(steps, 2));
            field.x = x;
            x += field.width + GAP;
         }
         for (; i < fields.length; i++)
         {
            TextField(fields[i]).visible = false;
         }
      }

      // Field `i`, made on first use, its label `label` translated when it changed.
      private function fieldAt(i:int, label:String) : TextField
      {
         if (i == fields.length)
         {
            var made:TextField = Text.field(SIZE, null, Text.BOLD);
            made.autoSize = TextFieldAutoSize.LEFT;
            addChild(made);
            fields.push(made);
            given.push(null);
            labels.push("");
         }
         if (given[i] != label)
         {
            given[i] = label;
            labels[i] = label.charAt(0) == "$" ? Text.translate(label) : label;
         }
         return TextField(fields[i]);
      }

      // Step `i` of `steps`, or `missing` when not given (by default never reached).
      private static function step(steps:Array, i:int, missing:Number = Number.POSITIVE_INFINITY) : Number
      {
         return steps != null && i < steps.length ? Number(steps[i]) : missing;
      }
   }
}
