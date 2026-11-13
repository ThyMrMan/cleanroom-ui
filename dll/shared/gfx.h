// Cleanroom UI: helpers every DLL of ours compiles in (gfx.cpp): reading Flash values, finding a menu's instance,
// the natives every movie of ours may call (TextureExists, HudColor), and checking a call site before hooking it.
#pragma once

#include "f4se/ScaleformCallbacks.h"
#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"

#include <initializer_list>
#include <string>

namespace crui {

// A Flash value as a number (int, uint, number or bool; else 0).
double Num(const GFxValue &v);

// Member `name` of Flash object `obj` into `out`; false when `obj` is not an object or lacks it.
bool Get(GFxValue &obj, const char *name, GFxValue *out);

// Log Flash object `obj`'s members by name and GFx type to the DLL's log (a menu not laid out as expected).
void LogMembers(GFxValue &obj);

// The first of `bases` (paths in `movie`) with a BGSCodeObj, and with member `needs` unless null: its path into
// `base`, it into `root` and its BGSCodeObj into `code`; false (base cleared) when none has them.
bool FindMenu(GFxMovieRoot *movie, std::initializer_list<const char *> bases, const char *needs, std::string *base,
              GFxValue *root, GFxValue *code);

// Listen for `event` on Flash object `target` with a native `Handler`; false when it could not be added.
template <class Handler>
bool Listen(GFxMovieRoot *movie, GFxValue &target, const char *event) {
    GFxValue args[2];
    movie->CreateString(&args[0], event);
    CreateFunction<Handler>(&args[1], movie);
    return target.Invoke("addEventListener", nullptr, args, 2);
}

// INI color `prefix`R/G/B:Interface (the player's HUD colors, e.g. iHUDColor) as 0xRRGGBB; white when not found.
UInt32 HudColor(const char *prefix);

// Give `plugin` (a movie's f4se.plugins.<our DLL>) the natives crui's Images and Theme call: TextureExists and
// HudColor (docs/swf_framework.md#images).
void RegisterSharedNatives(GFxMovieRoot *movie, GFxValue *plugin);

// Whether the call instruction at `site` still calls `target`.
bool CallsTarget(uintptr_t site, uintptr_t target);

}  // namespace crui
