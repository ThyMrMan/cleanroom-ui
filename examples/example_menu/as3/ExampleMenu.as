package
{
   import crui.Bridge;
   import crui.Draw;
   import crui.HintBar;
   import crui.Input;
   import crui.ScrollList;
   import crui.Text;
   import crui.Theme;
   import flash.display.MovieClip;
   import flash.display.Sprite;
   import flash.events.Event;
   import flash.text.TextField;

   // An example menu on Cleanroom UI (Interface/ExampleMenu.swf), opened by ExampleMenu.Open (papyrus/). A panel in
   // the player's HUD color with a title, a list of three items and the button hints; mouse, keyboard and a pad all
   // work it (crui's Input). Picking an item sends ExampleMenu_Pick (its index) to Papyrus; Back sends
   // ExampleMenu_Close. The title comes from Data/F4SE/Plugins/ExampleMenu.ini (MCM's copy over it), [General]
   // sTitle, through CleanroomUI.dll's Settings; without the DLL the menu still works, in the game's default color.
   // Copy this folder to start a menu of your own: rename ExampleMenu everywhere (the class, the file, the script,
   // the events).
   // See docs/api.md#example
   public class ExampleMenu extends MovieClip
   {
      // The panel's size and padding, and the list's rows.
      private static const WIDE:Number = 480;
      private static const HIGH:Number = 260;
      private static const PAD:Number = 20;
      private static const ITEMS:Array = [{text: "First item"}, {text: "Second item"}, {text: "Third item"}];
      private var bridge:Bridge = new Bridge();
      private var input:Input;
      private var list:ScrollList = new ScrollList();

      public function ExampleMenu()
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
         var panel:Sprite = new Sprite();
         panel.x = (stage.stageWidth - WIDE) / 2;
         panel.y = (stage.stageHeight - HIGH) / 2;
         Draw.box(panel.graphics, WIDE, HIGH, 0.9);
         addChild(panel);
         var title:TextField = Text.label(panel, PAD, PAD, 24);
         Text.show(title, settingsTitle());
         list.listWidth = WIDE - 2 * PAD;
         list.rows = ITEMS.length;
         list.x = PAD;
         list.y = PAD + 50;
         list.onPick = pick;
         panel.addChild(list);
         list.show(ITEMS, 0);
         input = new Input(stage, onAction);
         var hints:HintBar = new HintBar(input);
         hints.show([[Input.ACCEPT, "Pick"], [Input.BACK, "Close"]]);
         hints.x = PAD;
         hints.y = HIGH - PAD - 24;
         panel.addChild(hints);
      }

      // The title: [General] sTitle of ExampleMenu.ini, else a default.
      private function settingsTitle() : String
      {
         var settings:Object = bridge.settings("ExampleMenu");
         var general:Object = settings != null ? settings.General : null;
         return general != null && general.sTitle ? String(general.sTitle) : "Example Menu";
      }

      // FO4's menu input (the pad, and keys sent as controls), to Input.
      public function ProcessUserEvent(control:String, down:Boolean) : Boolean
      {
         return input != null && input.control(control, down);
      }

      // The game says which device is in use; the hints follow it.
      public function SetPlatform(platform:uint, ps3Switch:Boolean) : void
      {
         if (input != null)
         {
            input.setPlatform(platform);
         }
      }

      // Input's actions: Back closes, the rest work the list (up, down, accept, a digit).
      private function onAction(action:String, digit:int) : Boolean
      {
         if (action == Input.BACK)
         {
            bridge.send("ExampleMenu_Close");
            return true;
         }
         return list.action(action, digit);
      }

      private function pick(index:int) : void
      {
         bridge.send("ExampleMenu_Pick", index);
      }
   }
}
