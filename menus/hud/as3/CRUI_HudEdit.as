package
{
   import crui.Bridge;
   import crui.Draw;
   import crui.Text;
   import crui.Theme;
   import flash.display.MovieClip;
   import flash.display.Sprite;
   import flash.events.Event;
   import flash.events.KeyboardEvent;
   import flash.events.MouseEvent;
   import flash.geom.Rectangle;
   import flash.text.TextField;
   import flash.text.TextFieldAutoSize;
   import flash.ui.Keyboard;

   // The edit menu for our HUD host's widgets (Interface/CRUI_HudEdit.swf, opened by CRUI_Hud.EditLayout). Each frame
   // it draws, over the game dimmed, a box with its name where each widget's place puts it (CleanroomUI.dll's
   // HudLayout, from the host's reports, so the boxes are right while the HUD is paused under the menu). Drag a box to
   // move its widget (HudMove), turn the wheel over it to scale it (HudScale), press the reset key (R, a setting read
   // through HudKeys) to put the one under the cursor back where its mod puts it (HudReset); Esc or Tab saves the layout and closes (HudEditDone). Mouse and keyboard
   // only. The game's own HUD parts are here too, named by their clips. Widgets HUDFramework hosts are not here: its
   // own editor places them.
   // See docs/swf_framework.md#hud-edit
   public class CRUI_HudEdit extends MovieClip
   {
      // The DLL whose natives hold the layout (f4se.plugins.<DLL>).
      private static const DLL:String = "CleanroomUI";
      // A wheel notch's scale step, a box's smallest side (px), and the dimming over the game.
      private static const STEP:Number = 1.1;
      private static const SMALLEST:Number = 24;
      private static const DIM:Number = 0.35;
      private var bridge:Bridge = new Bridge();
      private var boxes:Sprite = new Sprite();
      private var hint:TextField;
      private var hinted:String = null;
      private var layout:Array = [];
      private var names:Array = [];
      private var dragging:String = null;
      private var lastX:Number = 0;
      private var lastY:Number = 0;
      // The reset key (HudKeys, from CleanroomUI.ini) and its name for the hint.
      private var resetKey:uint = 82;
      private var resetName:String = "R";

      public function CRUI_HudEdit()
      {
         super();
         addEventListener(Event.ADDED_TO_STAGE, onStage);
      }

      // F4SE hands the menu its f4se object as it opens.
      public function onF4SEObjCreated(codeObject:Object) : void
      {
         bridge.f4se = codeObject;
      }

      private function onStage(e:Event) : void
      {
         Theme.fromHud(bridge.shared());
         var dll:Object = bridge.natives(DLL);
         var keys:Object = dll != null && dll.HudKeys != null ? dll.HudKeys() : null;
         if (keys != null)
         {
            resetKey = uint(keys.reset);
            resetName = String(keys.resetName);
         }
         graphics.beginFill(0, DIM);
         graphics.drawRect(0, 0, stage.stageWidth, stage.stageHeight);
         graphics.endFill();
         addChild(boxes);
         hint = Text.field(20, null, Text.BOLD);
         hint.autoSize = TextFieldAutoSize.CENTER;
         hint.width = stage.stageWidth;
         hint.y = stage.stageHeight * 0.08;
         addChild(hint);
         stage.addEventListener(MouseEvent.MOUSE_DOWN, onDown);
         stage.addEventListener(MouseEvent.MOUSE_MOVE, onMove);
         stage.addEventListener(MouseEvent.MOUSE_UP, onUp);
         stage.addEventListener(MouseEvent.MOUSE_WHEEL, onWheel);
         stage.addEventListener(KeyboardEvent.KEY_DOWN, onKey);
         stage.focus = this;
         addEventListener(Event.ENTER_FRAME, onFrame);
      }

      // Each frame: every widget's box where its place puts it, and the hint (or that there is nothing to place).
      private function onFrame(e:Event) : void
      {
         var dll:Object = bridge.natives(DLL);
         layout = dll != null && dll.HudLayout != null ? dll.HudLayout() as Array : [];
         var says:String = layout.length > 0 ? "$CRUI_HUD_EDIT_HINT" : "$CRUI_HUD_EDIT_NONE";
         if (says != hinted)
         {
            hinted = says;
            Text.show(hint, says + "\t" + resetName);
         }
         boxes.graphics.clear();
         while (names.length > layout.length)
         {
            boxes.removeChild(TextField(names.pop()));
         }
         for (var i:int = 0; i < layout.length; i++)
         {
            var box:Rectangle = rectOf(layout[i]);
            Draw.outline(boxes.graphics, box.x, box.y, box.width, box.height, 2, layout[i].name == dragging ? 1 : 0.7);
            if (i == names.length)
            {
               var made:TextField = Text.field(16, null, Text.BOLD);
               made.autoSize = TextFieldAutoSize.LEFT;
               boxes.addChild(made);
               names.push(made);
            }
            var label:TextField = TextField(names[i]);
            if (label.text != shown(String(layout[i].name)))
            {
               Text.show(label, shown(String(layout[i].name)));
            }
            label.x = box.x;
            label.y = box.y - label.height;
         }
      }

      // How a name shows on its box: a HUD part ("part:HPMeter_mc") by its clip's name alone ("HPMeter").
      private static function shown(name:String) : String
      {
         if (name.indexOf("part:") == 0)
         {
            name = name.substr(5);
            return name.lastIndexOf("_mc") == name.length - 3 ? name.substr(0, name.length - 3) : name;
         }
         return name;
      }

      // Entry `entry`'s box on this stage (at least SMALLEST on a side, so an empty widget can still be caught).
      private function rectOf(entry:Object) : Rectangle
      {
         var wide:Number = stage.stageWidth, high:Number = stage.stageHeight;
         return new Rectangle(Number(entry.left) * wide, Number(entry.top) * high,
                              Math.max(SMALLEST, Number(entry.width) * wide),
                              Math.max(SMALLEST, Number(entry.height) * high));
      }

      // The name of the widget whose box is under the cursor (the last drawn, on top), or null.
      private function under() : String
      {
         for (var i:int = layout.length - 1; i >= 0; i--)
         {
            if (rectOf(layout[i]).contains(mouseX, mouseY))
            {
               return String(layout[i].name);
            }
         }
         return null;
      }

      private function onDown(e:MouseEvent) : void
      {
         dragging = under();
         lastX = mouseX;
         lastY = mouseY;
      }

      private function onMove(e:MouseEvent) : void
      {
         var dll:Object = bridge.natives(DLL);
         if (dragging == null || dll == null)
         {
            return;
         }
         dll.HudMove(dragging, (mouseX - lastX) / stage.stageWidth, (mouseY - lastY) / stage.stageHeight);
         lastX = mouseX;
         lastY = mouseY;
      }

      private function onUp(e:MouseEvent) : void
      {
         dragging = null;
      }

      private function onWheel(e:MouseEvent) : void
      {
         var dll:Object = bridge.natives(DLL);
         var name:String = under();
         if (name != null && dll != null && e.delta != 0)
         {
            dll.HudScale(name, e.delta > 0 ? STEP : 1 / STEP);
         }
      }

      // The reset key resets the widget under the cursor; Esc or Tab saves and closes.
      private function onKey(e:KeyboardEvent) : void
      {
         var dll:Object = bridge.natives(DLL);
         if (dll == null)
         {
            return;
         }
         if (e.keyCode == resetKey && under() != null)
         {
            dll.HudReset(under());
         }
         else if (e.keyCode == Keyboard.ESCAPE || e.keyCode == Keyboard.TAB)
         {
            dll.HudEditDone();
         }
      }
   }
}
