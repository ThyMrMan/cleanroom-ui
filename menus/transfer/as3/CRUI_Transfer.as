package
{
   import crui.Bridge;
   import flash.display.MovieClip;
   import flash.events.Event;

   // Our movie inside the container and barter menus. CleanroomUI.dll loads it into the menu instance that holds
   // ItemCard_mc with a Loader when either menu opens (transfer.cpp HookTransferMenu), and stamps each list line of an
   // item that wears with cruiCND, its own stack's condition (0 to 1), as the game builds the line. Each frame this
   // keeps a CND line at the highlighted line's cruiCND on the item card, when the card shows that line
   // (its InfoObj is the line's ItemCardInfoList). It takes no input.
   // See docs/swf_framework.md#transfer
   public class CRUI_Transfer extends MovieClip
   {
      // The DLL whose Log native we write to (f4se.plugins.<DLL>).
      private static const DLL:String = "CleanroomUI";
      // The CND line's text, translated by the card (as pipboy.cpp's LINE).
      private static const LINE:String = "$CRUI_CND";
      private static const STAMP:String = "cruiCND";
      // The menu's two lists: the container's or vendor's, and the player's.
      private static const LISTS:Array = [["ContainerList_mc"], ["PlayerInventory_mc", "PlayerList_mc"]];
      private var bridge:Bridge = new Bridge();
      private var menu:Object;
      private var seen:Object = {};

      public function CRUI_Transfer()
      {
         super();
         mouseEnabled = false;
         mouseChildren = false;
         addEventListener(Event.ADDED_TO_STAGE, onStage);
      }

      // Our Loader's parent is the menu instance; F4SE's object is on the menu movie's own root.
      private function onStage(e:Event) : void
      {
         menu = parent != null ? parent.parent : null;
         try
         {
            bridge.f4se = Object(stage.getChildAt(0)).f4se;
         }
         catch (error:Error)
         {
            bridge.f4se = null;
         }
         log("loaded into " + (menu != null ? String(menu) : "nothing"));
         addEventListener(Event.ENTER_FRAME, onFrame);
      }

      // Each frame: the highlighted line the card shows, when it has a condition, gets its CND line.
      private function onFrame(e:Event) : void
      {
         try
         {
            var card:Object = menu.ItemCard_mc;
            var lines:Array = card != null ? card.InfoObj as Array : null;
            var entry:Object = lines != null ? shownEntry(lines) : null;
            if (entry != null && keep(lines, Math.round(Number(entry[STAMP]) * 100)))
            {
               card.onDataChange();
               once("drew the CND line on an item card");
            }
         }
         catch (error:Error)
         {
            once("no item card: " + error.message);
         }
      }

      // The selected entry of either list whose ItemCardInfoList is the card's `lines` and which has a condition.
      private function shownEntry(lines:Array) : Object
      {
         for each (var path:Array in LISTS)
         {
            var entry:Object = selected(path);
            if (entry != null && entry.ItemCardInfoList === lines && entry.hasOwnProperty(STAMP))
            {
               return entry;
            }
         }
         return null;
      }

      // The selected entry of the list at `path` under the menu, or null when the menu has no such list.
      private function selected(path:Array) : Object
      {
         try
         {
            var at:Object = menu;
            for each (var name:String in path)
            {
               at = at != null ? at[name] : null;
            }
            return at != null ? at.selectedEntry : null;
         }
         catch (error:Error)
         {
            once("no " + path.join(".") + ": " + error.message);
         }
         return null;
      }

      // The CND line in card `lines` at `percent`, added or updated; true when either was needed.
      private static function keep(lines:Array, percent:Number) : Boolean
      {
         for each (var shown:Object in lines)
         {
            if (shown != null && shown.text == LINE)
            {
               if (shown.value == percent)
               {
                  return false;
               }
               shown.value = percent;
               return true;
            }
         }
         lines.push({text: LINE, value: percent, showAsPercent: true});
         return true;
      }

      private function once(what:String) : void
      {
         if (seen[what] == null)
         {
            seen[what] = true;
            log(what);
         }
      }

      private function log(text:String) : void
      {
         var dll:Object = bridge.natives(DLL);
         if (dll != null && dll.Log != null)
         {
            dll.Log("Transfer movie: " + text);
         }
      }
   }
}
