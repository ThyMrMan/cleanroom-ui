// Cleanroom UI on CommonLibF4: what every DLL of ours shares (base.cpp). The game's addresses by Address Library ID,
// turning a feature off when the running game lacks one; reading Flash values; registering movie natives; the
// natives every movie of ours may call (TextureExists, HudColor, Log); the log level; checking a call site.
// See docs/spin_offs.md#runtime
#pragma once

#include <cstdint>
#include <string>

namespace crui {

// A game address as Address Library IDs per runtime (old-gen 1.10.163, next-gen 1.10.980/984, 1.11.x) plus an
// offset inside it (a call site, a vtable slot). An ID of 0 is one not known for that runtime.
struct Ids {
    std::uint64_t og = 0, ng = 0, ae = 0;
    std::uintptr_t offset = 0;
};

// The running game's address of `ids`; 0 when its runtime's ID is unknown or its database lacks it (CommonLibF4's
// own lookup ends the game instead).
std::uintptr_t Address(const Ids &ids);

// The running game's address of `ids` as a function or object pointer of type `T`; null as Address gives 0.
template <class T>
T At(const Ids &ids) {
    return reinterpret_cast<T>(Address(ids));
}

// The member of type `T` at `offset` bytes into engine object `base`.
template <class T>
T &Field(void *base, std::size_t offset) {
    return *reinterpret_cast<T *>(static_cast<std::uint8_t *>(base) + offset);
}

// The runtime the game is ("1.11.240 (AE)"), for the log.
std::string RuntimeName();

// Whether this runtime is the one every engine layout and call site here was read in (Fallout4.exe 1.11.240); the
// hooks stay off on any other until they are checked there.
bool CheckedRuntime();

// The log level Data\F4SE\Plugins\<name>.ini gives ([Log] iLevel: 0 errors, 1 warnings, 2 messages (the default),
// 3 every open, list and key), for F4SE::Init.
REX::ELogLevel LogLevel(const char *name);

// A Flash value as a number (int, uint, number or bool; else 0).
double Num(const Scaleform::GFx::Value &v);

// A string value `text` made in `movie` (copied, so `text` may go away).
Scaleform::GFx::Value String(Scaleform::GFx::Movie *movie, const char *text);

// Give Flash object `plugin` in `movie` the native `T` (a Scaleform::GFx::FunctionHandler) as member `name`. One `T`
// serves every movie and lives as long as the game, as F4SE's own natives do.
template <class T>
void RegisterFunction(Scaleform::GFx::Value *plugin, Scaleform::GFx::Movie *movie, const char *name) {
    static T *handler = new T;
    Scaleform::GFx::Value function;
    movie->CreateFunction(&function, handler);
    plugin->SetMember(name, function);
}

// INI color `prefix`R/G/B:Interface (the player's HUD colors, e.g. iHUDColor) as 0xRRGGBB; white when not found.
std::uint32_t HudColor(const char *prefix);

// Give `plugin` (a movie's f4se.plugins.<our DLL>) the natives every movie of ours may call: TextureExists (crui's
// Images), HudColor([prefix]) (its Theme) and Log(text) (docs/swf_framework.md#images). A DLL registering its own
// native of one of these names does so after this call.
void RegisterSharedNatives(Scaleform::GFx::Movie *movie, Scaleform::GFx::Value *plugin);

// Whether the call instruction at `site` still calls `target` (both nonzero).
bool CallsTarget(std::uintptr_t site, std::uintptr_t target);

}  // namespace crui
