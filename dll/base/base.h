// Cleanroom UI on CommonLibF4: what every DLL of ours shares (base.cpp). The game's addresses by Address Library ID,
// turning a feature off when the running game lacks one; reading and building Flash values; finding a menu's movie
// and instance; listening on Flash objects; registering movie natives; the natives every movie of ours may call
// (TextureExists, HudColor, Log); the log level; checking a call site.
// See docs/spin_offs.md#runtime
#pragma once

#include <cstdint>
#include <initializer_list>
#include <string>

namespace crui {

using Scaleform::GFx::FunctionHandler;
using Scaleform::GFx::Movie;
using Scaleform::GFx::Value;

// A game address as Address Library IDs per runtime (old-gen 1.10.163, next-gen 1.10.980/984, 1.11.x) plus an
// offset inside it (a call site, a vtable slot), old-gen's own: code moves inside a function between the
// generations. An ID of 0 is one not known for that runtime. Vtables keep one ID in every database.
struct Ids {
    std::uint64_t og = 0, ng = 0, ae = 0;
    std::uintptr_t offset = 0, ogOffset = 0;
};

// An old-gen ID and a next-gen ID (also 1.11's), with `offset` into the next-gen one and `ogOffset` into the
// old-gen one (`offset` when not given). Old-gen IDs were matched in the 1.10.163 exe (docs/spin_offs.md#ids).
constexpr Ids Both(std::uint64_t og, std::uint64_t ng, std::uintptr_t offset = 0, std::uintptr_t ogOffset = ~0ull) {
    return {og, ng, ng, offset, ogOffset == ~0ull ? offset : ogOffset};
}

// Whether the game is old-gen (1.10.163), where a few patches write other bytes.
bool OldGen();

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

// Whether this runtime is one every engine layout and call site here was read in (Fallout4.exe 1.11.240, 1.10.163
// and 1.10.984); the hooks stay off on any other until they are checked there.
bool CheckedRuntime();

// The log level Data\F4SE\Plugins\<name>.ini gives ([Log] iLevel: 0 errors, 1 warnings, 2 messages (the default),
// 3 every open, list and key), for F4SE::Init.
REX::ELogLevel LogLevel(const char *name);

// A Flash value as a number (int, uint, number or bool; else 0).
double Num(const Value &v);

// A string value `text` made in `movie` (copied, so `text` may go away).
Value String(Movie *movie, const char *text);
inline Value String(Movie *movie, const std::string &text) {
    return String(movie, text.c_str());
}

// Member `name` of Flash object `obj` into `out`; false when `obj` is not an object or lacks it.
bool Get(const Value &obj, const char *name, Value *out);

// The Flash object `v` holds, to tell two values of one object apart from two objects; null when not an object.
const void *Identity(const Value &v);

// Log Flash object `obj`'s members by name and GFx type (a menu not laid out as expected).
void LogMembers(const Value &obj);

// The movie of the menu `name` while it is open; null otherwise.
Movie *MenuMovie(const char *name);

// The first of `bases` (paths in `movie`) with a BGSCodeObj, and with member `needs` unless null: its path into
// `base`, it into `root` and its BGSCodeObj into `code`; false (base cleared) when none has them.
bool FindMenu(Movie *movie, std::initializer_list<const char *> bases, const char *needs, std::string *base,
              Value *root, Value *code);

// A Flash function calling native `T` (a FunctionHandler) into `out`. One `T` serves every movie and lives as long
// as the game, as F4SE's own natives do.
template <class T>
void CreateFunction(Value *out, Movie *movie) {
    static T *handler = new T;
    movie->CreateFunction(out, handler);
}

// Give Flash object `plugin` in `movie` the native `T` as member `name`.
template <class T>
void RegisterFunction(Value *plugin, Movie *movie, const char *name) {
    Value function;
    CreateFunction<T>(&function, movie);
    plugin->SetMember(name, function);
}

// Listen for `event` on Flash object `target` with native `T`; false when it could not be added.
template <class T>
bool Listen(Movie *movie, Value &target, const char *event) {
    Value args[2] = {String(movie, event), Value()};
    CreateFunction<T>(&args[1], movie);
    return target.Invoke("addEventListener", nullptr, args, 2);
}

// Load movie `path` (under Interface) into Flash object `parent` with a Loader, as F4SE's UI.Load does; false when
// the Loader could not be added or started.
bool LoadChild(Movie *movie, Value &parent, const char *path);

// INI color `prefix`R/G/B:Interface (the player's HUD colors, e.g. iHUDColor) as 0xRRGGBB; white when not found.
std::uint32_t HudColor(const char *prefix);

// Give `plugin` (a movie's f4se.plugins.<our DLL>) the natives every movie of ours may call: TextureExists (crui's
// Images), HudColor([prefix]) (its Theme) and Log(text) (docs/swf_framework.md#images). A DLL registering its own
// native of one of these names does so after this call.
void RegisterSharedNatives(Movie *movie, Value *plugin);

// Whether the call instruction at `site` still calls `target` (both nonzero).
bool CallsTarget(std::uintptr_t site, std::uintptr_t target);

}  // namespace crui
