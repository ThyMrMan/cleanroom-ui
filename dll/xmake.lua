-- Cleanroom UI's F4SE plugins on CommonLibF4 (Dear-Modding-FO4), one DLL each for every Fallout 4 version that has
-- an Address Library database (docs/spin_offs.md#runtime): CleanroomUI.dll (the framework) and CleanroomDialogue.dll.
--
--   xmake f -p windows -a x64 -m release -y && xmake build -y
--
-- CRUI_COMMONLIBF4 names a CommonLibF4 checkout (its submodules in, patches/ applied by `python -m crui dll`);
-- the default is the template's beside this repo, tools_ext/commonlibf4-template/lib/commonlibf4.

local commonlib = os.getenv("CRUI_COMMONLIBF4")
    or path.join(os.scriptdir(), "..", "..", "tools_ext", "commonlibf4-template", "lib", "commonlibf4")
includes(commonlib)

set_project("cleanroom-ui")
set_version("1.0.0")
set_license("GPL-3.0")
set_languages("c++23")
set_warnings("allextra")
set_encodings("utf-8")

add_rules("mode.release", "mode.releasedbg")
-- The C++ runtime linked in: a player needs no Visual C++ redistributable.
set_runtimes("MT")
add_defines("COMMONLIB_RUNTIMECOUNT=3")

target("CleanroomUI")
    -- The framework's one version (framework/condition.h VERSION), so F4SE's plugin version is VERSION.0.0.
    on_load(function (target)
        local header = io.readfile(path.join(target:scriptdir(), "framework", "condition.h"))
        target:set("version", header:match("constexpr std::uint32_t VERSION = (%d+);") .. ".0.0")
    end)
    add_rules("commonlibf4.plugin", {
        name = "CleanroomUI",
        author = "cleanroom-ui",
        description = "Cleanroom UI's framework: item condition, Pip-Boy lines, lists and tabs, the HUD host"
    })
    add_files("base/*.cpp", "framework/*.cpp")
    add_headerfiles("base/*.h", "framework/*.h")
    add_includedirs("base", "framework")
    set_pcxxheader("base/pch.h")

target("CleanroomDialogue")
    add_rules("commonlibf4.plugin", {
        name = "CleanroomDialogue",
        author = "cleanroom-ui",
        description = "Cleanroom UI's dialogue menu: every option of a converted conversation"
    })
    add_files("base/*.cpp", "dialogue/*.cpp")
    add_headerfiles("base/*.h", "dialogue/*.h")
    add_includedirs("base", "dialogue")
    set_pcxxheader("base/pch.h")
