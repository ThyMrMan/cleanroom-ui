package
{
   import crui.Bridge;
   import crui.Draw;
   import crui.Input;
   import crui.Modal;
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
   import flash.utils.getTimer;

   // The edit menu for our HUD host's widgets (Interface/CRUI_HudEdit.swf, opened by CRUI_Hud.EditLayout). Each frame
   // it draws, over the game dimmed, a box with its name where each widget's place puts it (CleanroomUI.dll's
   // HudLayout, from the host's reports, so the boxes are right while the HUD is paused under the menu). Drag a box to
   // move its widget (HudMove), turn the wheel over it to scale it (HudScale), press the reset key (R, a setting read
   // through HudKeys) to put the one under the cursor back where its mod (or the game) puts it, shown, in its own
   // color (HudReset); the hide key (H) hides or shows it (HudHide), the color key (C) steps it through a few colors
   // (HudTint); hidden ones keep a dim box, so they can be shown again; the show key (V) opens a list of when it
   // shows (RuleList: the preset's rule, always, or the conditions picked; HudSetShow); the backup key (K) writes the
   // layout to its backup file (HudBackup) and the restore key (L) puts it back from there (HudRestore); Esc or Tab
   // closes (HudEditDone), the layout kept with the save (or, with [Hud] bLayoutPerSave=0, in the file every save
   // shares). With a gamepad (the game's word, SetPlatform, or Input's guess), one
   // box is picked instead of the one under the cursor: LB and RB pick the previous and next (top to bottom, then left
   // to right), the D-pad held moves it (faster the longer it is held), LT and RT scale it, X hides or shows it, Y gives
   // it the next color, A opens its rule list, View (Select) resets it, B saves and closes. The pad's buttons only
   // count while a pad is in use: FO4 sends some keyboard keys as the same controls (R is X, C is LB, V is View).
   // The game's own HUD parts are here too, named by their clips.
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
      // Boxes overlap (the subtitles over the compass): the cursor takes the smallest box under it, and a click
      // where the last one was (within STILL px) takes the next box under it, round the end; that one stays the
      // cursor's until it moves away.
      private static const STILL:Number = 4;
      private var cycled:String = null;
      private var cycleX:Number = -1000;
      private var cycleY:Number = -1000;
      // The reset, hide and color keys (HudKeys, from CleanroomUI.ini) and their names for the hint.
      private var resetKey:uint = 82;
      private var resetName:String = "R";
      private var hideKey:uint = 72;
      private var hideName:String = "H";
      private var colorKey:uint = 67;
      private var colorName:String = "C";
      private var showKey:uint = 86;
      private var showName:String = "V";
      private var backupKey:uint = 75;
      private var backupName:String = "K";
      private var restoreKey:uint = 76;
      private var restoreName:String = "L";
      // The second hint line: where the layout is kept and the backup keys, or for a while what a backup key did.
      private static const NOTICE_MS:int = 2500;
      private var keeping:TextField;
      private var notice:String = null;
      private var noticeUntil:int = 0;
      private var kept:String = null;
      // The rule list (the show key over a box) in its pop-up, the box it edits, and the keys it moves by (Input:
      // arrows, Enter, Esc or Tab to close).
      private var input:Input;
      private var modal:Modal = new Modal();
      private var rules:RuleList = new RuleList();
      private var editing:String = null;
      // The colors the color key steps through, from a widget's own (-1): white, FNV's amber, FO3's and FO4's
      // green, blue and red.
      private static const COLORS:Array = [-1, 0xFFFFFF, 0xFFB642, 0x1AFF80, 0x2EB4FF, 0xFF4646];
      // A hidden widget's box (alpha), and the word on its name.
      private static const HIDDEN_ALPHA:Number = 0.3;
      private var hiddenWord:String;
      // The pad's own actions (Input's for the rest), the D-pad's directions, how fast a held D-pad moves a box (px a
      // frame, from slow to fast over RAMP_MS), and the picked box's outline.
      private static const RESET:String = "reset";
      private static const SMALLER:String = "smaller";
      private static const BIGGER:String = "bigger";
      private static const DIRECTIONS:Object = {Up: [0, -1], Down: [0, 1], Left: [-1, 0], Right: [1, 0]};
      private static const MOVE_SLOW:Number = 1;
      private static const MOVE_FAST:Number = 12;
      private static const RAMP_MS:int = 1500;
      private static const PICKED_LINE:Number = 4;
      // The box the pad works on, the D-pad directions held, and since when.
      private var picked:String = null;
      private var held:Object = {};
      private var heldSince:int = 0;

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
            hideKey = uint(keys.hide);
            hideName = String(keys.hideName);
            colorKey = uint(keys.color);
            colorName = String(keys.colorName);
            showKey = uint(keys.show);
            showName = String(keys.showName);
            if (keys.backup != null)
            {
               backupKey = uint(keys.backup);
               backupName = String(keys.backupName);
               restoreKey = uint(keys.restore);
               restoreName = String(keys.restoreName);
            }
         }
         hiddenWord = Text.translate("$CRUI_HUD_EDIT_HIDDEN");
         graphics.beginFill(0, DIM);
         graphics.drawRect(0, 0, stage.stageWidth, stage.stageHeight);
         graphics.endFill();
         addChild(boxes);
         hint = Text.field(20, null, Text.BOLD);
         hint.autoSize = TextFieldAutoSize.CENTER;
         hint.width = stage.stageWidth;
         hint.y = stage.stageHeight * 0.08;
         addChild(hint);
         keeping = Text.field(16, null, Text.BOLD);
         keeping.autoSize = TextFieldAutoSize.CENTER;
         keeping.width = stage.stageWidth;
         keeping.y = hint.y + 30;
         addChild(keeping);
         rules.onChange = function(box:String, rule:String) : void
         {
            var dll:Object = bridge.natives(DLL);
            if (dll != null)
            {
               dll.HudSetShow(box, rule);
            }
         };
         rules.onDone = function() : void { modal.close(); };
         modal.handler = function(action:String, digit:int) : Boolean { return rules.list.action(action, digit); };
         modal.onClose = function() : void { editing = null; };
         addChild(modal);
         input = new Input(stage, onAction);
         // The pad's buttons, only while a pad is in use (FO4 sends R, C, Q, V and T as these controls too).
         input.mapControl("LShoulder", Input.PREV, true);
         input.mapControl("RShoulder", Input.NEXT, true);
         input.mapControl("LTrigger", SMALLER, true);
         input.mapControl("RTrigger", BIGGER, true);
         input.mapControl("XButton", Input.X, true);
         input.mapControl("YButton", Input.Y, true);
         input.mapControl("Select", RESET, true);
         input.watch(function() : void
         {
            hinted = null;
            held = {};
         });
         stage.addEventListener(MouseEvent.MOUSE_DOWN, onDown);
         stage.addEventListener(MouseEvent.MOUSE_MOVE, onMove);
         stage.addEventListener(MouseEvent.MOUSE_UP, onUp);
         stage.addEventListener(Event.MOUSE_LEAVE, onLeave);
         stage.addEventListener(MouseEvent.MOUSE_WHEEL, onWheel);
         stage.addEventListener(KeyboardEvent.KEY_DOWN, onKey);
         stage.focus = this;
         addEventListener(Event.ENTER_FRAME, onFrame);
      }

      // Each frame: every widget's box where its place puts it, and the hint (or that there is nothing to place); the
      // rule list's rule in force, which a pick changes (the preset's comes back from the DLL).
      private function onFrame(e:Event) : void
      {
         var dll:Object = bridge.natives(DLL);
         layout = dll != null && dll.HudLayout != null ? dll.HudLayout() as Array : [];
         if (editing != null && entryOf(editing) != null)
         {
            rules.refresh(String(entryOf(editing).show));
         }
         var pad:Boolean = input != null && input.gamepad;
         if (pad && layout.length > 0 && entryOf(picked) == null)
         {
            picked = String(inOrder()[0].name);
         }
         moveHeld(dll);
         var says:String = layout.length == 0 ? "$CRUI_HUD_EDIT_NONE"
                           : pad ? "$CRUI_HUD_EDIT_PAD_HINT" : "$CRUI_HUD_EDIT_HINT";
         if (says != hinted)
         {
            hinted = says;
            Text.show(hint, says + "\t" + resetName + "\t" + hideName + "\t" + colorName + "\t" + showName);
         }
         showKeeping(dll);
         boxes.graphics.clear();
         var hovered:String = pad || modal.isOpen ? null : under();
         while (names.length > layout.length)
         {
            boxes.removeChild(TextField(names.pop()));
         }
         for (var i:int = 0; i < layout.length; i++)
         {
            var box:Rectangle = rectOf(layout[i]);
            var hidden:Boolean = layout[i].hidden == true;
            var color:Number = Number(layout[i].color);
            var chosen:Boolean = layout[i].name == dragging || (pad ? layout[i].name == picked : layout[i].name == hovered);
            Draw.outline(boxes.graphics, box.x, box.y, box.width, box.height, pad && chosen ? PICKED_LINE : 2,
                         chosen ? 1 : hidden ? HIDDEN_ALPHA : 0.7, color >= 0 ? uint(color) : -1);
            if (i == names.length)
            {
               var made:TextField = Text.field(16, null, Text.BOLD);
               made.autoSize = TextFieldAutoSize.LEFT;
               boxes.addChild(made);
               names.push(made);
            }
            var label:TextField = TextField(names[i]);
            var rule:String = layout[i].show != null ? String(layout[i].show) : "";
            var text:String = shown(String(layout[i].name)) + (hidden ? " (" + hiddenWord + ")" : "")
                              + (rule != "" ? " [" + rule + "]" : "");
            if (label.text != text)
            {
               Text.show(label, text);
            }
            label.textColor = color >= 0 ? uint(color) : Theme.color;
            label.alpha = hidden ? 0.6 : 1;
            label.x = box.x;
            label.y = box.y - label.height;
         }
      }

      // The second hint line: what a backup key just did, for NOTICE_MS; else where the layout is kept and the keys.
      private function showKeeping(dll:Object) : void
      {
         if (notice != null && getTimer() > noticeUntil)
         {
            notice = null;
         }
         var perSave:Boolean = dll == null || dll.HudLayoutPerSave == null || dll.HudLayoutPerSave() == true;
         var says:String = notice != null ? notice
                           : (perSave ? "$CRUI_HUD_EDIT_PER_SAVE" : "$CRUI_HUD_EDIT_SHARED") + "\t" + backupName + "\t"
                             + restoreName;
         if (says != kept)
         {
            kept = says;
            Text.show(keeping, says);
         }
      }

      // Back the layout up to its file, or put it back from there, and say how it went for a while.
      private function backup(dll:Object, restore:Boolean) : void
      {
         var count:Number = Number(restore ? dll.HudRestore() : dll.HudBackup());
         notice = count < 0 ? (restore ? "$CRUI_HUD_EDIT_NO_BACKUP" : "$CRUI_HUD_EDIT_BACKUP_FAILED")
                  : (restore ? "$CRUI_HUD_EDIT_RESTORED" : "$CRUI_HUD_EDIT_BACKED_UP") + "\t" + count;
         noticeUntil = getTimer() + NOTICE_MS;
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

      // The names of the boxes under the cursor, smallest first (the one on top first among equals).
      private function stack() : Array
      {
         var found:Array = [];
         for (var i:int = layout.length - 1; i >= 0; i--)
         {
            var box:Rectangle = rectOf(layout[i]);
            if (box.contains(mouseX, mouseY))
            {
               found.push({name: String(layout[i].name), area: box.width * box.height, order: -i});
            }
         }
         found.sortOn(["area", "order"], Array.NUMERIC);
         var names:Array = [];
         for each (var entry:Object in found)
         {
            names.push(entry.name);
         }
         return names;
      }

      // Whether the cursor is still where the last click cycled.
      private function still() : Boolean
      {
         return Math.abs(mouseX - cycleX) <= STILL && Math.abs(mouseY - cycleY) <= STILL;
      }

      // The name of the widget the cursor works on: the one a click cycled to while the cursor stays there, else the
      // smallest box under it; null for none.
      private function under() : String
      {
         var names:Array = stack();
         if (cycled != null && still() && names.indexOf(cycled) >= 0)
         {
            return cycled;
         }
         return names.length > 0 ? String(names[0]) : null;
      }

      private function onDown(e:MouseEvent) : void
      {
         // While the rule list is open the mouse is its own (ScrollList).
         if (modal.isOpen)
         {
            dragging = null;
            return;
         }
         var names:Array = stack();
         var at:int = cycled != null && still() ? names.indexOf(cycled) : -1;
         cycled = names.length == 0 ? null : at >= 0 ? String(names[(at + 1) % names.length]) : String(names[0]);
         cycleX = mouseX;
         cycleY = mouseY;
         dragging = cycled;
         lastX = mouseX;
         lastY = mouseY;
      }

      // A drag moves its box by the mouse's move since the last; a move with the button up ends it (its release was
      // missed, off the movie), so a box never follows a cursor that isn't holding it (played 2026-11-15). The DLL
      // keeps the box on the screen (HudMove).
      private function onMove(e:MouseEvent) : void
      {
         var dll:Object = bridge.natives(DLL);
         if (dragging != null && !e.buttonDown)
         {
            dragging = null;
         }
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

      // The cursor left the movie: its release may never come, so the drag ends.
      private function onLeave(e:Event) : void
      {
         dragging = null;
      }

      private function onWheel(e:MouseEvent) : void
      {
         var dll:Object = bridge.natives(DLL);
         var name:String = modal.isOpen ? null : under();
         if (name != null && dll != null && e.delta != 0)
         {
            dll.HudScale(name, e.delta > 0 ? STEP : 1 / STEP);
         }
      }

      // Open the rule list for box `name`, in the middle of the screen.
      private function openRules(name:String) : void
      {
         var entry:Object = entryOf(name);
         editing = name;
         var dll:Object = bridge.natives(DLL);
         rules.edit(name, entry != null ? String(entry.showOwn) : "", entry != null ? String(entry.show) : "",
                    dll != null && dll.HudConditions != null ? dll.HudConditions() as Array : null);
         modal.x = (stage.stageWidth - (RuleList.WIDE + 2 * RuleList.PAD)) / 2;
         modal.y = (stage.stageHeight - rules.panelHeight) / 2;
         modal.open(rules, RuleList.WIDE + 2 * RuleList.PAD, rules.panelHeight);
         input.open();
      }

      // The entry for widget `name` in the layout, or null.
      private function entryOf(name:String) : Object
      {
         for each (var entry:Object in layout)
         {
            if (entry.name == name)
            {
               return entry;
            }
         }
         return null;
      }

      // The layout's entries top to bottom, then left to right, for the pad's LB and RB.
      private function inOrder() : Array
      {
         var out:Array = layout.concat();
         out.sortOn(["top", "left"], Array.NUMERIC);
         return out;
      }

      // Pick the box `by` places after the picked one in inOrder(), round the ends.
      private function pickBy(by:int) : void
      {
         var order:Array = inOrder();
         var at:int = 0;
         for (var i:int = 0; i < order.length; i++)
         {
            if (order[i].name == picked)
            {
               at = i;
            }
         }
         picked = String(order[(at + by + order.length) % order.length].name);
      }

      // Each frame: the picked box moved by the D-pad directions held, faster the longer they are.
      private function moveHeld(dll:Object) : void
      {
         var dx:Number = 0, dy:Number = 0;
         for (var control:String in held)
         {
            dx += DIRECTIONS[control][0];
            dy += DIRECTIONS[control][1];
         }
         if ((dx == 0 && dy == 0) || picked == null || dll == null || modal.isOpen)
         {
            return;
         }
         var speed:Number = MOVE_SLOW + (MOVE_FAST - MOVE_SLOW) * Math.min(1, (getTimer() - heldSince) / RAMP_MS);
         dll.HudMove(picked, dx * speed / stage.stageWidth, dy * speed / stage.stageHeight);
      }

      // FO4's menu input. With a pad and no list open, the D-pad's presses and releases are kept to move the picked
      // box each frame; the rest goes to Input (its actions to onAction).
      public function ProcessUserEvent(control:String, down:Boolean) : Boolean
      {
         if (input != null && input.gamepad && !modal.isOpen && DIRECTIONS[control] != null)
         {
            if (down)
            {
               var none:Boolean = true;
               for (var any:String in held)
               {
                  none = false;
               }
               heldSince = none ? getTimer() : heldSince;
               held[control] = true;
            }
            else
            {
               delete held[control];
            }
            return true;
         }
         return input != null && input.control(control, down);
      }

      public function SetPlatform(platform:uint, ps3Switch:Boolean) : void
      {
         if (input != null)
         {
            input.setPlatform(platform);
         }
      }

      // Input's actions: the rule list's while it is open; else, with a pad, the pad's on the picked box (the
      // keyboard and mouse have their own: onKey and the mouse handlers).
      private function onAction(action:String, digit:int) : Boolean
      {
         if (modal.isOpen)
         {
            return modal.action(action, digit);
         }
         var dll:Object = bridge.natives(DLL);
         if (!input.gamepad || dll == null || picked == null)
         {
            return false;
         }
         switch (action)
         {
            case Input.PREV:
            case Input.NEXT:
               pickBy(action == Input.NEXT ? 1 : -1);
               break;
            case SMALLER:
            case BIGGER:
               dll.HudScale(picked, action == BIGGER ? STEP : 1 / STEP);
               break;
            case Input.X:
               dll.HudHide(picked);
               break;
            case Input.Y:
               recolor(dll, picked);
               break;
            case RESET:
               dll.HudReset(picked);
               break;
            case Input.ACCEPT:
               openRules(picked);
               break;
            case Input.BACK:
               dll.HudEditDone();
               break;
            default:
               return false;
         }
         return true;
      }

      // Give box `name` the next of COLORS.
      private function recolor(dll:Object, name:String) : void
      {
         var at:int = COLORS.indexOf(int(entryOf(name).color));
         dll.HudTint(name, COLORS[(at + 1) % COLORS.length]);
      }

      // Over a widget: the reset key resets it, the hide key hides or shows it, the color key gives it the next
      // color. Anywhere: the backup and restore keys. Esc or Tab closes.
      private function onKey(e:KeyboardEvent) : void
      {
         var dll:Object = bridge.natives(DLL);
         if (dll == null)
         {
            return;
         }
         if (modal.isOpen)
         {
            // The rule list's keys are Input's (Esc or Tab closes it, not the menu).
            return;
         }
         if (e.keyCode == backupKey || e.keyCode == restoreKey)
         {
            backup(dll, e.keyCode == restoreKey);
            return;
         }
         var name:String = under();
         if (name != null && e.keyCode == showKey)
         {
            openRules(name);
         }
         else if (name != null && e.keyCode == resetKey)
         {
            dll.HudReset(name);
         }
         else if (name != null && e.keyCode == hideKey)
         {
            dll.HudHide(name);
         }
         else if (name != null && e.keyCode == colorKey)
         {
            recolor(dll, name);
         }
         else if (e.keyCode == Keyboard.ESCAPE || e.keyCode == Keyboard.TAB)
         {
            dll.HudEditDone();
         }
      }
   }
}
