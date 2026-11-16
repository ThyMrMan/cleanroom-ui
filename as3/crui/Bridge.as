package crui
{
   import flash.display.Stage;

   // The movie's link to the game: F4SE's root object (handed over in onF4SEObjCreated) and our DLL's natives.
   // See docs/fork/swf_framework.md#design
   public class Bridge
   {
      public var f4se:Object;

      // F4SE hands each item of a Papyrus Var array over wrapped as {__var__: value}: the plain values.
      public static function plain(items:Array) : Array
      {
         var out:Array = [];
         for each (var item:* in items)
         {
            out.push(item != null && typeof item == "object" && "__var__" in item ? item["__var__"] : item);
         }
         return out;
      }

      // The natives a plugin registered on every movie (f4se.plugins.<name>); null without F4SE or the plugin.
      public function natives(plugin:String) : Object
      {
         return f4se != null && f4se.plugins != null ? f4se.plugins[plugin] : null;
      }

      // The natives every DLL of ours gives each movie (TextureExists, HudColor): CleanroomUI.dll's, else
      // CleanroomDialogue.dll's; null with neither.
      public function shared() : Object
      {
         var dll:Object = natives("CleanroomUI");
         return dll != null ? dll : natives("CleanroomDialogue");
      }

      // For a movie loaded into a menu (ours or another mod's: docs/swf_framework.md#inject): F4SE's object, which is
      // on the menu movie's own root; null without F4SE. Call it once the movie is on the stage.
      public function attach(stage:Stage) : void
      {
         try
         {
            f4se = Object(stage.getChildAt(0)).f4se;
         }
         catch (error:Error)
         {
            f4se = null;
         }
      }

      // A mod's INI files as {section: {setting: value}} (Data/F4SE/Plugins/<mod>.ini, MCM's copy over it), each
      // value typed by its prefix (b, i, u, f; else text); null without our DLL. See docs/swf_framework.md#settings
      public function settings(mod:String) : Object
      {
         var dll:Object = shared();
         return dll != null && dll.Settings != null ? dll.Settings(mod) : null;
      }

      // Send an F4SE external event (Papyrus RegisterForExternalEvent); nothing before F4SE is there.
      public function send(event:String, ... args) : void
      {
         if (f4se != null)
         {
            f4se.SendExternalEvent.apply(null, [event].concat(args));
         }
      }

      // Mount a DDS under Textures (path without that folder) as img://<name>; false without F4SE.
      // MountImage returns true even for a missing file, and images drop when the menu closes.
      public function mount(menu:String, path:String, name:String) : Boolean
      {
         return f4se != null && f4se.MountImage(menu, path, name);
      }
   }
}
