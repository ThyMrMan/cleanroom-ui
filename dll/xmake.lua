-- Cleanroom UI's F4SE plugins on CommonLibF4 (Dear-Modding-FO4), one DLL for every Fallout 4 version that has an
-- Address Library database (docs/spin_offs.md#runtime). CleanroomDialogue.dll is built here; CleanroomUI.dll still
-- builds on F4SE 0.7.9 with build.bat until it is ported.
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
-- The C++ runtime linked in, as build.bat's /MT: a player needs no Visual C++ redistributable.
set_runtimes("MT")
add_defines("COMMONLIB_RUNTIMECOUNT=3")

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
