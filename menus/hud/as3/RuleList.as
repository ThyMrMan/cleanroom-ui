package
{
   import crui.ScrollList;
   import flash.display.Sprite;

   // The edit menu's rule list (the show key, V, over a box): when the box is shown. Rows: the preset's rule, always
   // shown, then each condition (on or off), then Done. Picking the preset's or always sets that; picking a condition
   // switches it in the box's own rule, starting from the rule in force (the preset's, when the box had none of its
   // own); switching the last one off leaves it always shown. A threshold already on low (low:0.3) stays while low
   // stays on. Each pick goes to `onChange(box, rule)` at once (CleanroomUI.dll's HudSetShow: "" the preset's,
   // "always" none). Other mods' conditions (HudConditions: CRUI_Hud.SetCondition) follow ours, under their labels.
   // See docs/swf_framework.md#hud-conditions
   public class RuleList extends Sprite
   {
      // The conditions, in the list's order, and each row's words.
      public static const CONDITIONS:Array = ["combat", "drawn", "aiming", "sneaking", "vats", "changed", "low",
                                              "moved", "peek"];
      private static const WORDS:Object = {
         combat: "$CRUI_SHOW_COMBAT", drawn: "$CRUI_SHOW_DRAWN", aiming: "$CRUI_SHOW_AIMING",
         sneaking: "$CRUI_SHOW_SNEAKING", vats: "$CRUI_SHOW_VATS", changed: "$CRUI_SHOW_CHANGED", low: "$CRUI_SHOW_LOW",
         moved: "$CRUI_SHOW_MOVED", peek: "$CRUI_SHOW_PEEK"};
      private static const PRESET:int = 0;
      private static const ALWAYS:int = 1;
      private static const FIRST:int = 2;
      private static const ON:String = "$CRUI_SHOW_ON";
      // The list's width and rows, and the space round it in its panel.
      public static const WIDE:Number = 520;
      public static const PAD:Number = 12;
      // Rows shown at most (the rest scroll).
      private static const ROWS:int = 14;
      public var list:ScrollList = new ScrollList();
      // function(box:String, rule:String):void, a rule picked; function():void, Done picked.
      public var onChange:Function;
      public var onDone:Function;
      private var box:String;
      // The box's own rule ("" none: the preset's; "always"), and the rule in force (the edit menu's layout).
      private var own:String = "";
      private var inForce:String = "";
      // The conditions listed now: ours, then the mods' (by name); the mods' labels.
      private var names:Array = CONDITIONS.concat();
      private var labels:Object = {};

      public function RuleList()
      {
         super();
         list.rows = ROWS;
         list.listWidth = WIDE;
         list.line = 30;
         list.x = PAD;
         list.y = PAD;
         list.onPick = pick;
         addChild(list);
      }

      // Edit box `name`'s rule: its own `own` and the one in force `inForce`; `mods` the mods' conditions
      // ([{name, label}], HudConditions; null for none).
      public function edit(name:String, own:String, inForce:String, mods:Array = null) : void
      {
         box = name;
         names = CONDITIONS.concat();
         labels = {};
         for each (var mod:Object in mods || [])
         {
            names.push(String(mod.name));
            labels[String(mod.name)] = String(mod.label);
         }
         this.own = own != null ? own : "";
         this.inForce = inForce != null ? inForce : "";
         list.show(items(), 0);
      }

      // The rule in force now (from the edit menu's layout, each frame): redrawn when it changed.
      public function refresh(inForce:String) : void
      {
         inForce = inForce != null ? inForce : "";
         if (inForce != this.inForce)
         {
            this.inForce = inForce;
            list.show(items(), list.selected);
         }
      }

      // The panel's height round the list.
      public function get panelHeight() : Number
      {
         return list.listHeight + 2 * PAD;
      }

      private function items() : Array
      {
         var out:Array = [{text: "$CRUI_SHOW_PRESET", value: own == "" ? ON : ""},
                          {text: "$CRUI_SHOW_ALWAYS", value: own == "always" ? ON : ""}];
         var now:Array = conditions();
         for each (var condition:String in names)
         {
            out.push({text: WORDS[condition] || labels[condition], value: has(now, condition) ? ON : ""});
         }
         out.push({text: "$CRUI_SHOW_DONE", value: ""});
         return out;
      }

      // The conditions in force: the box's own, else the preset's (as tokens: "low:0.3" keeps its threshold).
      private function conditions() : Array
      {
         var rule:String = own != "" ? own : inForce;
         return rule == "" || rule == "always" ? [] : rule.split(",");
      }

      private static function has(tokens:Array, condition:String) : Boolean
      {
         for each (var token:String in tokens)
         {
            if (token.split(":")[0] == condition)
            {
               return true;
            }
         }
         return false;
      }

      private function pick(index:int) : void
      {
         if (index == FIRST + names.length)
         {
            if (onDone != null)
            {
               onDone();
            }
            return;
         }
         if (index == PRESET || index == ALWAYS)
         {
            own = index == PRESET ? "" : "always";
         }
         else
         {
            var condition:String = names[index - FIRST];
            var kept:Array = [];
            var had:Boolean = false;
            for each (var token:String in conditions())
            {
               if (token.split(":")[0] == condition)
               {
                  had = true;
               }
               else
               {
                  kept.push(token);
               }
            }
            if (!had)
            {
               kept.push(condition);
            }
            own = kept.length > 0 ? kept.join(",") : "always";
         }
         if (own != "")
         {
            inForce = own == "always" ? "" : own;
         }
         if (onChange != null)
         {
            onChange(box, own);
         }
         list.show(items(), index);
      }
   }
}
