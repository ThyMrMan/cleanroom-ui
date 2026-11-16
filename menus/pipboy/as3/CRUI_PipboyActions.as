package
{
   import crui.Bridge;
   import crui.HintBar;
   import crui.Input;
   import crui.Modal;
   import crui.ScrollList;
   import crui.Theme;
   import flash.display.MovieClip;
   import flash.events.Event;

   // The Pip-Boy's action menu on a pad (Interface/CRUI_PipboyActions.swf), which CRUI_PipboyActions.Open opens over
   // the Pip-Boy as View is pressed there (CleanroomUI.dll's pad.cpp). It lists the actions in use (PipboyActions:
   // Repair, Ammo) in a pop-up; A picks one, which presses that action's key in the Pip-Boy (PipboyPress), and B
   // closes. Once a pick opens a list, the pop-up goes and the D-pad, A and B work the list as the Pip-Boy's up, down,
   // Enter and Esc keys, until the list closes (PipboyListOpen); then the menu closes (PipboyActionsDone). A pick that
   // opens nothing (the item card says why) closes it at once. On top of the Pip-Boy, it takes the pad's input.
   // See docs/swf_framework.md#pipboy-pad
   public class CRUI_PipboyActions extends MovieClip
   {
      // The DLL whose natives run the Pip-Boy's lists (f4se.plugins.<DLL>).
      private static const DLL:String = "CleanroomUI";
      // The Pip-Boy keys Input's actions become while a list is open (Flash key codes: up, down, Enter, Esc).
      private static const KEYS:Object = {up: 38, down: 40, accept: 13, back: 27};
      // The pop-up's list width and padding, and how far down the screen it sits.
      private static const WIDE:Number = 320;
      private static const PAD:Number = 12;
      private static const DOWN:Number = 0.3;
      private var bridge:Bridge = new Bridge();
      private var input:Input;
      private var hints:HintBar;
      private var modal:Modal = new Modal();
      private var list:ScrollList = new ScrollList();
      private var actions:Array = [];
      // Whether a picked action's list is open, which the pad then works.
      private var working:Boolean = false;

      public function CRUI_PipboyActions()
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
         actions = dll != null ? dll.PipboyActions() as Array : null;
         if (actions == null || actions.length == 0)
         {
            done();
            return;
         }
         list.listWidth = WIDE;
         list.rows = actions.length;
         list.x = PAD;
         list.y = PAD;
         list.onPick = pick;
         list.show(actions, 0);
         modal.x = (stage.stageWidth - WIDE) / 2 - PAD;
         modal.y = stage.stageHeight * DOWN;
         modal.handler = list.action;
         modal.onClose = done;
         addChild(modal);
         modal.open(list, WIDE + 2 * PAD, list.listHeight + 2 * PAD);
         input = new Input(stage, onAction);
         hints = new HintBar(input);
         hints.show([[Input.ACCEPT, "$CRUI_PAD_CHOOSE"], [Input.BACK, "$CRUI_PAD_BACK"]]);
         hints.x = (stage.stageWidth - hints.width) / 2;
         hints.y = modal.y + list.listHeight + 2 * PAD + PAD;
         addChild(hints);
         addEventListener(Event.ENTER_FRAME, onFrame);
      }

      // FO4's menu input, to Input (its actions to onAction).
      public function ProcessUserEvent(control:String, down:Boolean) : Boolean
      {
         return input != null && input.control(control, down);
      }

      public function SetPlatform(platform:uint, ps3Switch:Boolean) : void
      {
         if (input != null)
         {
            input.setPlatform(platform);
         }
      }

      // Input's actions: the open list's, as the Pip-Boy's keys; else the pop-up's.
      private function onAction(action:String, digit:int) : Boolean
      {
         var dll:Object = bridge.natives(DLL);
         if (!working)
         {
            return modal.action(action, digit);
         }
         if (KEYS[action] == null || dll == null)
         {
            return false;
         }
         dll.PipboyPress(KEYS[action]);
         return true;
      }

      // Action `index` picked: press its key in the Pip-Boy; work its list when one opened, else close.
      private function pick(index:int) : void
      {
         var dll:Object = bridge.natives(DLL);
         working = dll != null && dll.PipboyPress(int(actions[index].key));
         if (!working)
         {
            done();
            return;
         }
         modal.onClose = null;
         modal.close();
         hints.y = stage.stageHeight * (1 - DOWN / 2);
      }

      // Each frame while a list is worked: close once it has closed.
      private function onFrame(e:Event) : void
      {
         var dll:Object = bridge.natives(DLL);
         if (working && (dll == null || !dll.PipboyListOpen()))
         {
            done();
         }
      }

      private function done() : void
      {
         removeEventListener(Event.ENTER_FRAME, onFrame);
         var dll:Object = bridge.natives(DLL);
         if (dll != null)
         {
            dll.PipboyActionsDone();
         }
      }
   }
}
