package
{
   import crui.Bridge;
   import crui.Theme;
   import flash.display.MovieClip;
   import flash.events.Event;
   import flash.utils.getTimer;

   // Our stats lines as one HUDFramework widget (Interface/CRUI_HudStats.swf, registered by CRUI_Hud.AddStats while
   // HUDFramework is installed): HUDFramework loads and places it, and the player moves, scales and fades it with
   // its editor. Each frame it draws every stats line hosted there, one under another in the order they were added,
   // from CleanroomUI.dll (HudWidgets, HudStats), as our own host draws its lines. It takes no messages.
   // See docs/swf_framework.md#hud
   public class CRUI_HudStats extends MovieClip
   {
      // The DLL whose natives give the stats lines (f4se.plugins.<DLL>).
      private static const DLL:String = "CleanroomUI";
      // How long a flashing value stays on, then off (ms), and the height of a line.
      private static const FLASH_MS:int = 400;
      private static const LINE:Number = 28;
      private var bridge:Bridge = new Bridge();
      private var version:Number = -1;
      // The stats lines shown here, in order, and each one's StatsLine.
      private var names:Array = [];
      private var lines:Object = {};

      public function CRUI_HudStats()
      {
         super();
         mouseEnabled = false;
         mouseChildren = false;
         addEventListener(Event.ADDED_TO_STAGE, onStage);
      }

      // HUDFramework's widget method: this widget takes no messages.
      public function processMessage(command:String, params:Array) : void
      {
      }

      // HUDFramework loads us into the HUD movie, whose root holds F4SE's object.
      private function onStage(e:Event) : void
      {
         try
         {
            bridge.f4se = Object(stage.getChildAt(0)).f4se;
         }
         catch (error:Error)
         {
            bridge.f4se = null;
         }
         addEventListener(Event.ENTER_FRAME, onFrame);
      }

      private function onFrame(e:Event) : void
      {
         var dll:Object = bridge.natives(DLL);
         if (dll == null)
         {
            return;
         }
         var list:Object = dll.HudWidgets(version);
         if (list != null)
         {
            sync(list);
         }
         var normal:uint = Theme.fromHud(dll);
         var warning:uint = uint(dll.HudColor("iHUDColorWarning"));
         var flashOff:Boolean = int(getTimer() / FLASH_MS) % 2 == 1;
         for each (var name:String in names)
         {
            StatsLine(lines[name]).update(dll.HudStats(name), normal, warning, flashOff);
         }
      }

      // Keep a line for each stats line of `list` hosted by HUDFramework, one under another.
      private function sync(list:Object) : void
      {
         version = Number(list.version);
         var wanted:Array = [];
         for each (var entry:Object in list.widgets as Array)
         {
            if (entry.movie == "" && entry.host == "HUDFramework")
            {
               wanted.push(String(entry.name));
            }
         }
         for each (var name:String in names)
         {
            if (wanted.indexOf(name) < 0)
            {
               removeChild(StatsLine(lines[name]));
               delete lines[name];
            }
         }
         names = wanted;
         for (var i:int = 0; i < names.length; i++)
         {
            var line:StatsLine = lines[names[i]] as StatsLine;
            if (line == null)
            {
               line = new StatsLine();
               lines[names[i]] = line;
               addChild(line);
            }
            line.y = i * LINE;
         }
      }
   }
}
