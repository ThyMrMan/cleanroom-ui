package
{
   import crui.Bridge;
   import crui.Theme;
   import flash.display.DisplayObject;
   import flash.display.DisplayObjectContainer;
   import flash.display.Loader;
   import flash.display.MovieClip;
   import flash.events.Event;
   import flash.events.IOErrorEvent;
   import flash.geom.Point;
   import flash.geom.Rectangle;
   import flash.net.URLRequest;
   import flash.system.ApplicationDomain;
   import flash.system.LoaderContext;
   import flash.utils.getTimer;

   // Our HUD host. CleanroomUI.dll loads it into the HUD movie (hud.cpp) once a mod adds a widget through CRUI_Hud,
   // under any HUDMenu.swf. Each frame it asks CleanroomUI.dll for the widget list when it changed (HudWidgets) and
   // loads each widget's movie (into the HUD's own class domain, as HUDFramework loads its widgets) or makes a stats
   // line, under the HUD part the widget names; hands each widget the messages sent to it (HudMessages) through
   // processMessage(command, params), HUDFramework's widget method, held until the widget has loaded; and redraws
   // the stats lines (HudStats). Each widget is moved and scaled by the place the player gave it in the edit menu,
   // and where its mod puts it is reported for that menu (HudReport). With a DEF_HUD.xml layout (HudPartLayout) it
   // also lays out the game's own HUD parts (PartLayout), again every few frames. It takes no input.
   // See docs/swf_framework.md#hud
   public class CRUI_Hud extends MovieClip
   {
      // The DLL whose natives give the widgets (f4se.plugins.<DLL>).
      private static const DLL:String = "CleanroomUI";
      // How long a flashing stats value stays on, then off (ms), and how often every widget is placed and reported
      // again (frames), for a widget whose size changed.
      private static const FLASH_MS:int = 400;
      private static const REPLACE_FRAMES:int = 15;
      private var bridge:Bridge = new Bridge();
      private var hud:Object;
      private var version:Number = -1;
      // Widget name -> {entry (as HudWidgets gave it), view (its Loader or StatsLine), corner (its HUD part's
      // top-left, measured before the widget joined it), content (its loaded movie), pending (messages held until
      // it loads)}.
      private var widgets:Object = {};
      private var seen:Object = {};
      private var frames:int = 0;
      // The game's HUD parts' layout (none until the DLL is found).
      private var parts:PartLayout;

      public function CRUI_Hud()
      {
         super();
         mouseEnabled = false;
         mouseChildren = false;
         addEventListener(Event.ADDED_TO_STAGE, onStage);
      }

      // Our Loader's parent is the HUD's root, which holds F4SE's object.
      private function onStage(e:Event) : void
      {
         hud = stage.getChildAt(0);
         try
         {
            bridge.f4se = hud.f4se;
         }
         catch (error:Error)
         {
            bridge.f4se = null;
         }
         log("loaded into the HUD (CleanroomUI.dll " + (bridge.natives(DLL) != null ? "found" : "not found")
             + ", HUDFramework " + (part("HUDFramework") != null ? "found" : "not found") + ")");
         addEventListener(Event.ENTER_FRAME, onFrame);
      }

      // Each frame: the widget list when it changed, the messages sent since, and the stats lines; the HUD parts'
      // layout on the first frame and every few after.
      private function onFrame(e:Event) : void
      {
         var dll:Object = bridge.natives(DLL);
         if (dll == null)
         {
            return;
         }
         if (parts == null)
         {
            parts = new PartLayout(dll.HudPartLayout != null ? dll.HudPartLayout() : null, log);
            parts.apply(hud);
         }
         else if (frames % REPLACE_FRAMES == 0)
         {
            parts.apply(hud);
         }
         var list:Object = dll.HudWidgets(version);
         if (list != null)
         {
            sync(dll, list);
         }
         else if (++frames % REPLACE_FRAMES == 0)
         {
            for each (var shown:Object in widgets)
            {
               place(dll, shown);
            }
         }
         for each (var message:Object in dll.HudMessages() as Array)
         {
            deliver(message);
         }
         drawStats(dll);
      }

      // Show our widgets of `list` ({version, widgets}; HUDFramework hosts those naming it): add the new ones, re-add
      // one whose movie or HUD part changed, move the rest, and take off those no longer listed.
      private function sync(dll:Object, list:Object) : void
      {
         version = Number(list.version);
         var wanted:Object = {};
         for each (var entry:Object in list.widgets as Array)
         {
            if (entry.host != "")
            {
               continue;
            }
            wanted[entry.name] = true;
            var shown:Object = widgets[entry.name];
            if (shown != null && (shown.entry.movie != entry.movie || shown.entry.anchor != entry.anchor))
            {
               remove(entry.name);
               shown = null;
            }
            if (shown == null)
            {
               shown = add(entry);
            }
            shown.entry = entry;
            place(dll, shown);
         }
         var gone:Array = [];
         for (var name:String in widgets)
         {
            if (!wanted[name])
            {
               gone.push(name);
            }
         }
         for each (name in gone)
         {
            remove(name);
         }
      }

      // Put widget `shown` where its mod puts it and report that to CleanroomUI.dll (origin and bounds, in fractions of
      // the screen), then move and scale it by the place the player gave it.
      private function place(dll:Object, shown:Object) : void
      {
         var view:DisplayObject = shown.view;
         var entry:Object = shown.entry;
         view.scaleX = view.scaleY = 1;
         view.x = shown.corner.x + Number(entry.x);
         view.y = shown.corner.y + Number(entry.y);
         if (stage == null || view.parent == null)
         {
            return;
         }
         var wide:Number = stage.stageWidth, high:Number = stage.stageHeight;
         var origin:Point = view.parent.localToGlobal(new Point(view.x, view.y));
         var bounds:Rectangle = view.getBounds(stage);
         dll.HudReport(String(entry.name), origin.x / wide, origin.y / high, bounds.x / wide, bounds.y / high,
                       bounds.width / wide, bounds.height / high);
         var at:Object = entry.place;
         if (at != null)
         {
            var moved:Point = view.parent.globalToLocal(new Point(origin.x + Number(at.dx) * wide,
                                                                  origin.y + Number(at.dy) * high));
            view.x = moved.x;
            view.y = moved.y;
            view.scaleX = view.scaleY = Number(at.scale);
         }
      }

      // Widget `entry` added under its HUD part: a stats line, or its movie loading.
      private function add(entry:Object) : Object
      {
         var shown:Object = {"entry":entry, "view":null, "corner":null, "content":null, "pending":[]};
         var parent:DisplayObjectContainer = anchor(String(entry.anchor));
         shown.corner = parent == this ? new Rectangle() : parent.getBounds(parent);
         if (entry.movie == "")
         {
            shown.view = new StatsLine();
         }
         else
         {
            var loader:Loader = new Loader();
            loader.contentLoaderInfo.addEventListener(Event.COMPLETE, function(e:Event) : void
            {
               loaded(shown, loader);
            });
            loader.contentLoaderInfo.addEventListener(IOErrorEvent.IO_ERROR, function(e:IOErrorEvent) : void
            {
               log("could not load " + entry.movie + " for widget " + entry.name);
            });
            loader.load(new URLRequest(String(entry.movie)), new LoaderContext(false, ApplicationDomain.currentDomain));
            shown.view = loader;
         }
         parent.addChild(shown.view);
         widgets[entry.name] = shown;
         log("added widget " + entry.name + (entry.movie == "" ? " (stats)" : " (" + entry.movie + ")")
             + (parent == this ? "" : " under " + entry.anchor));
         return shown;
      }

      // Widget `shown`'s movie has loaded: hand it the messages held for it.
      private function loaded(shown:Object, loader:Loader) : void
      {
         shown.content = loader.content;
         for each (var message:Object in shown.pending)
         {
            call(shown, message);
         }
         shown.pending = [];
      }

      // Take widget `name` off the HUD, its movie unloaded.
      private function remove(name:String) : void
      {
         var shown:Object = widgets[name];
         var loader:Loader = shown.view as Loader;
         if (loader != null)
         {
            loader.unloadAndStop();
         }
         if (shown.view.parent != null)
         {
            shown.view.parent.removeChild(shown.view);
         }
         delete widgets[name];
      }

      // Hand `message` ({name, command, params}) to its widget, or hold it until the widget's movie has loaded.
      private function deliver(message:Object) : void
      {
         var shown:Object = widgets[message.name];
         if (shown == null || shown.view is StatsLine)
         {
            once("a message (" + message.command + ") for " + message.name + ", which is no widget movie");
         }
         else if (shown.content == null)
         {
            shown.pending.push(message);
         }
         else
         {
            call(shown, message);
         }
      }

      private function call(shown:Object, message:Object) : void
      {
         try
         {
            shown.content.processMessage(String(message.command), message.params as Array);
         }
         catch (error:Error)
         {
            once("widget " + shown.entry.name + " took no message " + message.command + ": " + error.message);
         }
      }

      // Redraw every stats line in the HUD's colors, its flashing values on or off by the clock.
      private function drawStats(dll:Object) : void
      {
         var normal:uint = 0, warning:uint = 0, colored:Boolean = false;
         var flashOff:Boolean = int(getTimer() / FLASH_MS) % 2 == 1;
         for (var name:String in widgets)
         {
            var line:StatsLine = widgets[name].view as StatsLine;
            if (line == null)
            {
               continue;
            }
            if (!colored)
            {
               normal = Theme.fromHud(dll);
               warning = uint(dll.HudColor("iHUDColorWarning"));
               colored = true;
            }
            line.update(dll.HudStats(name), normal, warning, flashOff);
         }
      }

      // The HUD part at `path` (clip names from the HUD's root, dotted), or this host for "" or a part not there.
      private function anchor(path:String) : DisplayObjectContainer
      {
         if (path == "")
         {
            return this;
         }
         var found:DisplayObjectContainer = part(path) as DisplayObjectContainer;
         if (found == null)
         {
            once("no HUD part " + path + ": its widgets sit on the HUD itself");
         }
         return found != null ? found : this;
      }

      // The member of the HUD's root at dotted `path`; null when a step is missing.
      private function part(path:String) : Object
      {
         var at:Object = hud;
         for each (var step:String in path.split("."))
         {
            try
            {
               at = at != null ? at[step] : null;
            }
            catch (error:Error)
            {
               at = null;
            }
         }
         return at;
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
            dll.Log("HUD movie: " + text);
         }
      }
   }
}
