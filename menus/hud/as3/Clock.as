package
{
   import crui.Text;
   import flash.display.Sprite;
   import flash.text.TextField;
   import flash.text.TextFieldAutoSize;

   // Our clock ([Hud] bClock): the game's time, and with bClockRealTime the real time under it, in the HUD's bold font
   // and color, 12-hour or (bClock24) 24-hour. It sits in the HUD group TopRightGroup_mc, so it hides and fades with
   // it, and is laid out, placed and shown on conditions as the game's own parts are (PartLayout adds and feeds it).
   // It keeps the size of its widest time so far, so its box doesn't change as the minutes go by.
   // See docs/swf_framework.md#hud-extras
   public class Clock extends Sprite
   {
      // Its name in its group, and among the places and in the edit menu ("part:" + it), and its group.
      public static const NAME:String = "CRUI_Clock_mc";
      public static const GROUP:String = "TopRightGroup_mc";
      // The game time's size and the real time's (as StatsLine's), and the gap between them (px of the group).
      private static const SIZE:int = 20;
      private static const SMALL:int = 16;
      private static const GAP:Number = 2;
      private var game:TextField;
      private var real:TextField;
      private var shownGame:String = null;
      private var shownReal:String = null;
      private var shownColor:int = -1;

      public function Clock()
      {
         super();
         name = NAME;
         mouseEnabled = false;
         mouseChildren = false;
         game = Text.field(SIZE, null, Text.BOLD);
         game.autoSize = TextFieldAutoSize.LEFT;
         real = Text.field(SMALL, null, Text.BOLD);
         real.autoSize = TextFieldAutoSize.LEFT;
         addChild(game);
         addChild(real);
      }

      // Show game hour `hour` (0 to 24; below 0: not known, "--:--"), and the real time `now` when `withReal`, in
      // `color`, 24-hour when `twentyFour`.
      public function update(hour:Number, now:Date, withReal:Boolean, twentyFour:Boolean, color:uint) : void
      {
         var gameText:String = hour >= 0 ? format(hour, twentyFour) : "--:--";
         var realText:String = withReal ? format(now.hours + now.minutes / 60, twentyFour) : "";
         if (gameText == shownGame && realText == shownReal && int(color) == shownColor)
         {
            return;
         }
         shownGame = gameText;
         shownReal = realText;
         shownColor = int(color);
         Text.show(game, gameText);
         game.textColor = color;
         real.visible = withReal;
         if (withReal)
         {
            Text.show(real, "$CRUI_CLOCK_REAL\t" + realText);
            real.textColor = color;
            real.y = game.height + GAP;
         }
      }

      // `hour` (0 to 24) as "8:05 PM", or "20:05" when `twentyFour`.
      public static function format(hour:Number, twentyFour:Boolean) : String
      {
         var minutes:int = int(Math.floor(hour * 60)) % (24 * 60);
         var hours:int = int(minutes / 60);
         var mins:String = (minutes % 60 < 10 ? "0" : "") + String(minutes % 60);
         if (twentyFour)
         {
            return (hours < 10 ? "0" : "") + String(hours) + ":" + mins;
         }
         var twelve:int = hours % 12 == 0 ? 12 : hours % 12;
         return String(twelve) + ":" + mins + (hours < 12 ? " AM" : " PM");
      }
   }
}
