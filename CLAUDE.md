# CLAUDE.md: Internal Cheat (by BigH), CS2 Offline Learning Trainer (External-First)

This file is the project's memory. Each session the user will say "read CLAUDE.md, then do task X", so a fresh session
must be able to work from this file alone. **Keep it accurate.** See the Maintenance rule (§16) at the bottom.

---

## 1. Scope and rules (read first, never violate)

I'm continuing a personal learning project on reverse engineering and low-level Windows programming, using
**Counter-Strike 2 (CS2)**, launched via Steam with `-insecure`.

- CS2 is closed-source, 64-bit, Source 2, DirectX 11. VAC is **disabled by `-insecure`**. Trusted Mode is off.
- I only play it **offline, single-player, against bots, on my own PC**. I never join a VAC-secured server with the
  tool running.
- **The project is external-first.** A user-mode console/exe that uses `ReadProcessMemory` and `WriteProcessMemory`
  against `cs2.exe`. **No DLL injection, no hooks, no MinHook, no in-process code** until (and unless) a later phase
  explicitly says so.
- **Learning goals:** process memory reading, pointer chains, entity system traversal (the chunked Source 2 layout),
  `CreateInterface` resolution from outside the process, schema system lookups, pattern/signature scanning for
  update-proof offsets, world-to-screen maths, and an overlay renderer (ImGui + DX11 or a transparent Win32 window).
- **Out of scope, permanently:** multiplayer or online use, VAC/Trusted Mode bypass or evasion, DMA, manual mapping,
  module/thread hiding, obfuscation, packing, string encryption, anti-debug, network code, telemetry, distribution,
  installers, licensing, "how to share this".
  **If a task ever seems to drift toward any of these, stop and tell the user.**

What that means in practice:
- The tool opens a handle to `cs2.exe` with `PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION`
  (read-only where possible; write only for the specific features that need it).
- No `LoadLibraryW`, no `CreateRemoteThread`, no `SetWindowsHookEx`, no `NtCreateThreadEx`, no APC injection.
- No driver, no IOMMU games, no PCIe tricks.
- No code that talks to a network. Nothing.
- No feature that only makes sense against humans or on a VAC-secured server.
- **Never** test against an online match. `-insecure` only, offline bots only.
- If the user ever says "I'll just try it on a casual for one round": **stop and tell them no.**

---

## 2. Session start protocol and how to work in this repo

1. Read this whole file.
2. Check **§14 Current status** to see where we are and what comes next.
3. Confirm your understanding to the user in 2–3 lines (where we are, what the task is).
4. Plan before coding: list the files you'll touch and why. Ask if anything is unclear. Don't guess.
5. **One phase per session.** Don't start the next phase on your own.
6. **Ask before big architectural changes** (new module boundaries, new dependencies, threading changes, changing
   the render approach, switching from external to internal).
7. At the end of the task:
   1. Build Debug|x64 **and** Release|x64 with **zero warnings** (`/W4 /WX` makes warnings errors anyway).
      Run the unit tests (once the tests project exists).
   2. Summarise what changed, then tell the user **exactly** what to do in-game to test it and what they should see.
   3. Update the phase checkboxes (§13), **Current status** (§14), the **Decision log** (§15), the file tree (§5),
      any gotchas you learned (§10), `docs/offsets.md`, and add a dated entry to `docs/DEVLOG.md`.
   4. **Stop and wait for the user's OK.** Only after approval, commit: small, focused commits with clear messages.
      **Commits and PRs must not include any Claude/AI co-author, "Generated with" or attribution lines.** The user
      is the sole author. Repo: private.

If you can't build here (e.g. no Visual Studio on the machine running the session), say so plainly. Never claim a
build or test passed when it wasn't run.

---

## 3. Project overview and end goal

**Internal Cheat - by BigH (CS2 Offline, External)** (working name): an external trainer for CS2 with an in-game
overlay menu, running **only** under `-insecure` against bots.

The goal is a learning project, not parity with any public cheat and not a release. Feature targets, in order of
implementation:

- **Overlay menu** toggled with **INSERT**: a Home dashboard and sidebar pages (**ESP, Aimbot, Triggerbot, Player,
  Misc, Settings**). Help in (?) tooltips, adjustable menu size, settings applied live.
- **ESP** drawn in the overlay: 2D box, corner box, head circle, approximate skeleton (Phase 4+, needs bones),
  name, health bar/number, distance, weapon name, snaplines (origin choice), team mode, enemies-only, colour
  picker with opacity.
- **Aimbot:** hold/toggle key, head/body target, priority (crosshair / distance / lowest health), FOV radius +
  circle, smoothing, team check, max distance.
- **Triggerbot:** fires automatically with a living enemy under the crosshair, with a reaction delay; team check
  optional; hold key optional.
- **Player values:** health, armour, ammo per weapon (write-only features, gated behind a setting), each with
  Set / Freeze / hotkey.
- **Radar / misc:** enlarged radar, enemy dots on radar, bomb timer, spectator list, hitsound (via the overlay's
  own audio, not the game's).
- **Keybinds** for every action, **profiles** saved as JSON, **panic key**, clean shutdown.
- **Presets** (Off / Chill / Medium / Rage) that switch features and strengths.

**What external cannot do, and we accept that:**
- No silent aim (that requires a hook inside `CreateMove`).
- No weapon code patches (no spread, no recoil via patching).
- No calling game functions (TraceLine, etc.) — visible-only is a heuristic only, not a game-thread call.
- No sub-tick angle writes.
- If the user ever wants those, **that's a separate Phase 12+ decision** with a full re-scope, not a silent slide
  into internal.

**Reference projects (read-only, for understanding, not for copying whole):**
- **Osiris** (public CS2 internal): study the entity system walk, schema lookups, interface names, bone matrix
  reading. **The technique transfers even if the injection doesn't.** Write my own code.
- **`a2x/cs2-dumper`** (offset dumper): the authoritative source for `dwEntityList`, `dwLocalPlayerController`,
  `dwLocalPlayerPawn`, `dwViewMatrix`, `dwGameRules`, `dwGlobalVars`. Subscribe to it.
- **`sezzyaep/CS2-OFFSETS`** (offset mirror): second source to cross-check a2x.
- **`xsip/cs2-sdk-offsets-interfaces-dump`**: schema field offsets and interface names/vtables. The one to use
  for inside-entity offsets.
- **`scros22/cs2-universal-offsets`**: a newer external SDK generator with a live web browser at `cs2-sdk.com`.
- **`Leksa667/cs2-lks-dumper-cpp`**: a C++ port of the dumper if you want to modify the dumper itself.
- **The AC 1.3.0.2 internal project** (this same user's earlier repo): the architecture, module split, test
  strategy, keybind engine, and JSON profile shape carry over. Reuse the *patterns*, not the code.

**Everything stays offline. `-insecure` only. No VAC servers. Ever.**

---

## 4. Tech stack and build configuration

| What | Choice |
|---|---|
| IDE / compiler | MSVC toolset **v143** (VS 2022's toolset), built with **Visual Studio Community 2026** |
| Language | **C++20** (`/std:c++20`) |
| Platform | **x64 ONLY.** CS2 is 64-bit. There is no x86 configuration, on purpose |
| Projects | `external` (console exe → `cs2_external.exe`), `tests` (→ `tests.exe`), `vendor` (static lib → `vendor.lib`) |
| CRT | **Static** (`/MTd` Debug, `/MT` Release): no VC++ runtime DLL needed |
| UI | **Dear ImGui v1.92.x** with the **Win32 + DX11** backends (for the overlay's own window), vendored under `external/imgui/` |
| Overlay window | A transparent, click-through, topmost Win32 window with its own DX11 swap chain. **No DLL injection, no hooking of the game's Present.** We render our own window on top of the game |
| Memory access | `ReadProcessMemory` / `WriteProcessMemory` from a user-mode handle |
| Tests | `tests` console exe compiling the pure modules; **doctest v2.5.3** (header-only, `external/doctest/`) |
| JSON | **nlohmann/json v3.12.0** (single header, `external/nlohmann/`) |

**Don't add dependencies without asking the user first.** Vendored code lives in `external/`. Every vendored library
gets a line in `external/README.md` (version, licence, source tag, blob hash).

### Build configuration (`props/common.props`, imported by every project)
- `/std:c++20`, `/W4`, `/WX` (warnings are errors), linker `/WX`, `/sdl`, `/permissive-`, `/utf-8`, `/Zc:__cplusplus`,
  `/EHsc`, `/MP`.
- Defines: `WIN32`, `_WIN64`, `WIN32_LEAN_AND_MEAN`, `NOMINMAX`, plus `_DEBUG` / `NDEBUG`. Character set: Unicode.
- Output: `bin\<Configuration>\`. Intermediates: `obj\<Project>\<Configuration>\`. Both git-ignored.
- `WindowsTargetPlatformVersion` = `10.0` (latest installed Windows SDK; no SDK version pinned).
- Solution platform is `x64`.
- No `.vcxproj.filters` files: use **Solution Explorer → Show All Files**.
- Change shared settings in `props/common.props`, not in individual projects.
- **ImGui config defines** live in `props/imgui.props` (imported by `vendor` and `external`), never in `imconfig.h`.
- Third-party code is compiled in its own static library project (`external/vendor.vcxproj`).

### Build and run
```powershell
# repo root. Full MSBuild path:
$msb = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"
& $msb cs2-external.sln /m /nologo /v:minimal /p:Configuration=Debug   /p:Platform=x64
& $msb cs2-external.sln /m /nologo /v:minimal /p:Configuration=Release /p:Platform=x64

# unit tests (built by the solution, no game needed):
bin\Debug\tests.exe

# start CS2 first (Steam → CS2 → Properties → Launch Options):
#   -insecure -console -novid -nojoy
# Join a "Practice with Bots" match, then:
bin\Release\cs2_external.exe
```
- Run the external tool as **administrator** if `OpenProcess` fails (the game runs as admin on some setups).
- The overlay is a separate window. It sits topmost over the game and is click-through until you press INSERT.
- No DLL, no injection, no unload sequence beyond closing the exe.

### Machine notes (fill in on first session)
- **Visual Studio Community 2026** with the **Desktop development with C++** workload.
- Toolsets installed side by side: v143 and v145. The projects use **v143**.
- Windows SDK: latest installed (10.0.26100.0 or newer) via `WindowsTargetPlatformVersion 10.0`.
- **`msbuild` is not on PATH** in the user's terminal. Use the full path above (find with `vswhere`).
- **Steam** installed, CS2 installed, `-insecure` verified working (no VAC prompt, bots load).
- **a2x/cs2-dumper** run successfully; output at `docs/dumps/offsets.json` and `docs/dumps/client_dll.json`.

---

## 5. File tree

Status markers: ✅ exists, 🔲 planned (phase number in brackets).

```
cs2-external/
  CLAUDE.md                       ✅ this file: project memory and rules
  README.md                       ✅ [11] educational README
  .gitignore                      ✅ VS/C++ build output, .vs, local profiles/logs
  .gitattributes                  ✅ CRLF for VS files, text normalisation
  cs2-external.sln                ✅ solution: Debug|x64, Release|x64
  props/
    common.props                  ✅ shared build settings (C++20, x64, /W4 /WX, /MT, output dirs)
    imgui.props                   ✅ ImGui config defines + include paths
  docs/
    DEVLOG.md                     ✅ dated log
    offsets.md                    ✅ running notes on every offset, signature, schema field, with proof
    dumps/
      offsets.json                ✅ a2x/cs2-dumper output (module globals)
      client_dll.json             ✅ a2x/cs2-dumper output (schema classes + fields)
      interfaces.json             ✅ a2x/cs2-dumper output (interface names + vtables)
      buttons.json                ✅ a2x/cs2-dumper output (button bitflags)
  external/
    README.md                     ✅ what's vendored, versions, licences
    vendor.vcxproj                ✅ static lib compiling Dear ImGui
    imgui/                        ✅ Dear ImGui v1.92.x + Win32 + DX11 backends
    doctest/                      ✅ doctest v2.5.3
    nlohmann/                     ✅ nlohmann/json v3.12.0
  profiles/
    default.json                  ✅ [8] mirror of built-in defaults
  src/
    external/
      external.vcxproj            ✅ console exe, x64, static CRT
      main.cpp                    ✅ entry: find cs2.exe → open handle → init overlay → main loop
      core/
        process.h/.cpp            ✅ find_process, open_handle, module_base
        memory.h/.cpp             ✅ safe_read<T>, safe_write<T>, is_plausible_pointer, RPM/WPM wrappers
        pattern.h/.cpp            ✅ [2] signature scanner over a remote module
        log.h                     ✅ logger::info/warn/error
        runtime.h                 ✅ shared atomics (shutdown_requested)
      game/                       (THE ONLY place that dereferences game memory)
        offsets.h                 ✅ module RVAs (from dumps/offsets.json)
        schema.h                  ✅ class field offsets (from dumps/client_dll.json)
        interfaces.h/.cpp         ✅ [2] CreateInterface resolution from outside the process
        structs.h                 ✅ [3] Vec3, PlayerSnapshot, accessor helpers
        player.h/.cpp             ✅ [3] make_snapshot, validity checks, collect_players
        entities.h/.cpp           ✅ [3] walk the chunked entity list remotely
        handle.h/.cpp             ✅ [3] CHandle → entity pointer resolution
        view.h/.cpp               ✅ [3] view matrix read
        bones.h/.cpp              ✅ [4] bone array read (needs a self-found offset, see §7)
        weapon.h/.cpp             ✅ [3] read weapon id / name
        globals.h/.cpp            ✅ [3] CGlobalVars reads (curtime, maxClients, interval_per_tick)
      maths/                      (PURE)
        vec.h                     ✅ Vec2, Vec3 + ops
        angles.h/.cpp             ✅ Angles, normalize, calc_aim_angles, angular_distance, smoothing
        projection.h/.cpp         ✅ ViewMatrix, world_to_screen (row-major, matching this build)
        skeleton.h/.cpp           ✅ [4] joint specs, bone index → screen positions
      features/                   (PURE: data in, decisions out)
        esp.h/.cpp                ✅ health_colour, screen_box, display_name, build_esp → primitives
        aimbot.h/.cpp             ✅ aim_point, find_candidates, select_target, compute_aim
        triggerbot.h/.cpp         ✅ Triggerbot state machine
        player_values.h/.cpp      ✅ [6] plan_writes (health/armour/ammo)
      render/
        primitives.h              ✅ Line, Rect, FilledRect, Circle, Text, Primitive variant (PURE data)
        painter.h/.cpp            ✅ paint(primitives, font) on ImGui background draw list
      input/
        cursor.h/.cpp             ✅ CursorControl for the overlay window
        keys.h/.cpp               ✅ VK ↔ names, KeySet
        actions.h/.cpp            ✅ ActionId registry
        keybinds.h/.cpp           ✅ HOLD/TOGGLE/PRESS engine
        key_poll.h/.cpp           ✅ GetAsyncKeyState polling
        bind_capture.h/.cpp       ✅ bind capture
      settings/
        settings.h                ✅ Settings struct
        profile_json.h/.cpp       ✅ [8]
        profile_store.h/.cpp      ✅ [8]
        presets.h/.cpp            ✅ [8]
      ui/
        imgui_layer.h/.cpp        ✅ ImGuiLayer: context + Win32 + DX11 backends for the overlay window
        overlay_window.h/.cpp     ✅ transparent, click-through, topmost window; toggling click-through on INSERT
        theme.h/.cpp              ✅ palette
        widgets.h/.cpp            ✅ shared widgets
        menu.h/.cpp               ✅ sidebar + pages
        pages/                    ✅ one file per page (home, esp, aimbot, triggerbot, player, misc, settings)
      color.h                     ✅ Color (RGBA floats)
      config.h                    ✅ non-offset constants, Range<T> + every setting range
  tests/
    tests.vcxproj                 ✅ console exe (doctest)
    main.cpp                      ✅ DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
    core/test_pattern.cpp         ✅ [2] pattern parse, wildcard match, scan
    core/test_memory.cpp          ✅ safe_read / safe_write / pointer sanity
    maths/test_angles.cpp         ✅
    maths/test_projection.cpp     ✅
    maths/test_skeleton.cpp       ✅
    game/test_player.cpp          ✅
    game/test_entities.cpp        ✅ fake remote reader
    game/test_handle.cpp          ✅ [3] CHandle resolution
    features/test_esp.cpp         ✅
    features/test_aimbot.cpp      ✅
    features/test_triggerbot.cpp  ✅
    features/test_player_values.cpp ✅
    settings/test_settings.cpp    ✅
    settings/test_profile_json.cpp ✅
    settings/test_profile_store.cpp ✅
    settings/test_presets.cpp     ✅
    input/test_keys.cpp           ✅
    input/test_keybinds.cpp       ✅
    input/test_bind_capture.cpp   ✅
    test_color.cpp                ✅
```

---

## 6. Architecture

### 6.1 Module responsibilities and dependency rules
- **Pure modules** (no `<Windows.h>`, no ImGui, no raw game pointers): `maths/`, `features/`,
  `render/primitives.h`, `game/snapshot.h`, `input/keys|actions|keybinds`, `settings/`, `config.h`, `game/offsets.h`,
  `game/schema.h`. They take plain data and return plain data, so the `tests` project can compile and test them
  without the game.
- **`core/`** owns the process handle, memory read/write, pattern scanning, and logging. **No DLL lifetime code**
  (there is no DLL).
- **`game/`** is the only code that reads or writes game memory. It turns remote pointers into `PlayerSnapshot`
  copies and applies writes that features asked for.
- **`game/interfaces` + `game/schema`** are the only code that resolves interfaces and schema field offsets
  remotely.
- **ImGui** is included only in `ui/` and `render/painter`. The UI **never touches game memory**: it edits
  `Settings` and queues requests, and the main loop applies them.
- **`main.cpp`** is the orchestrator: find the process, init the overlay, run the frame loop (read → features →
  writes → draw), handle shutdown.

### 6.2 Threading model
| Thread | Who | Allowed to |
|---|---|---|
| Main thread | ours (`main.cpp`) | everything: process handle, game reads, feature logic, overlay rendering, keybinds |

- **External has one thread.** No DLL lifetime problem, no hook in-flight problem, no cross-thread state to
  synchronise. This is the single biggest engineering simplification over internal.
- The game runs on its own threads; we only read its memory from our process. We never call into the game.
- The overlay window has its own message pump on the main thread (standard Win32). ImGui is driven from there.

### 6.3 Data flow (one frame, Phase 1+)
```
main loop (60+ Hz, or vsync-limited):
  1. pump Win32 messages for the overlay window
  2. keybinds: GetAsyncKeyState → engine → ActionStates
  3. read:    game/ → GameState {local PlayerSnapshot, players, view matrix, globals} (copies)
              (all reads go through core::safe_read<T>, which is __try/__except-guarded)
  4. features (pure): aimbot → angles (write via WriteProcessMemory) · player_values → writes · esp → primitives
  5. apply:   game/ writes
  6. draw:    ImGui NewFrame → render/painter(primitives) on background draw list → ui::menu (if open)
              → ImGui Render → ImGui_ImplDX11_RenderDrawData on the overlay's swap chain
  7. Present the overlay swap chain
```

### 6.4 Shutdown (no unload sequence)
1. User presses the unload key (DELETE) or closes the overlay window.
2. Restore everything we changed in the game (frozen player values stop being written; nothing else is changed
   because there are no patches or hooks).
3. Shut down ImGui and the DX11 overlay.
4. Close the process handle.
5. Exit.

**There is no trampoline, no in-flight counter, no `FreeLibraryAndExitThread`. External is dramatically simpler
to shut down safely.**

### 6.5 Settings and keybinds (same design as AC)
- One `Settings` root (pure structs): `general`, `aimbot`, `triggerbot`, `esp`, `player`, `keybinds`.
- Numeric ranges defined once (`config.h`) and used by the UI sliders and the JSON clamping.
- Colours stored as RGBA floats in memory, `"#RRGGBBAA"` in JSON.
- Profiles live in `profiles\` next to the exe; `default` is built in.
- Keybinds: `BindMode` HOLD / TOGGLE / PRESS, keyboard + mouse buttons.
- Presets change feature switches and strengths only, never keybinds, colours, player values, or general settings.

---

## 7. Offsets, signatures and struct reconstruction

All offsets and signatures live in `src/external/game/offsets.h` and `src/external/game/schema.h`. Game:
**Counter-Strike 2**, module `client.dll` (the entity system, local player, view matrix, game rules, and global
vars are there). Some things are in `engine2.dll`.

**CS2 offsets change on every game update.** This is the single biggest difference from AssaultCube. Do not hardcode
absolute addresses for anything that changes. Use **schema fields** for inside-entity offsets and **signatures** for
module-base RVAs.

### Where the offsets come from
Do not find the stable offsets by hand. They are dumped automatically after every CS2 patch by the community:

- **`a2x/cs2-dumper`** — the reference. Subscribe to it. Its `output/offsets.json` and `output/client_dll.json`
  are the source of truth. The user has already run it and the output is in `docs/dumps/`.
- **`sezzyaep/CS2-OFFSETS`** — a mirror explicitly advertised as "always updated." Cross-check a2x against this.
- **`xsip/cs2-sdk-offsets-interfaces-dump`** — schema field offsets and interface names + vtables. The one to use
  for inside-entity fields.
- **`scros22/cs2-universal-offsets`** — a newer external SDK generator with a live web browser at `cs2-sdk.com`.

Copy the values you need into `offsets.h` and `schema.h`, and **log the source (repo + commit + date) next to each
one** so the provenance is clear.

### Three kinds of offsets

1. **Stable module RVAs:** `dwEntityList`, `dwLocalPlayerController`, `dwLocalPlayerPawn`, `dwViewMatrix`,
   `dwGlobalVars`, `dwGameRules`, `dwPlantedC4`, `dwCSGOInput`. These come straight from the dumper. They change
   on updates but slowly.
2. **Schema field offsets:** the offsets of fields *inside* `C_CSPlayerPawn` (health, team, position, bone matrix
   pointer, etc.). These move the most, and they are resolved from `client_dll.json` (or read at runtime from the
   game's own schema system, if the remote walk is viable).
3. **Signature-scanned RVAs:** store an IDA-style byte pattern in `offsets.h`, scan `client.dll` for it, get the
   address. Survives almost every update unless the pattern itself changes. Use this for anything the dumpers
   don't cover (most notably the bone array pointer — see below).

### Offset change rule
You may change an offset **only if** you:
1. **Prove it first** with a diagnostic (in-game snapshot diff, Cheat Engine, x64dbg, or a new dumper run) showing
   the old value is wrong and the new one is right. If the proof needs the user in-game, write the diagnostic and
   ask them to run it.
2. **Keep the old value as a comment** next to the new one, plus the date and the dumper commit it came from.
3. **Record** old value, new value, evidence and reason in the Decision log (§15) and `docs/offsets.md`.
4. **Flag it** at the very top of the phase summary as **"OFFSET CHANGED"**.

**Never change an offset based on a guess alone.**

### What the dumper gives you (already in `docs/dumps/`)
- `offsets.json`: the module globals. 29 offsets across `client.dll`, `engine2.dll`, `inputsystem.dll`,
  `matchmaking.dll`, `soundsystem.dll`. Every `dw*` symbol you'll reference.
- `client_dll.json`: 3301 classes and 569 enums, with every field offset. This is where health, team, position,
  eye angles, weapon handle, player name, and everything else lives.
- `interfaces.json`: 111 interfaces across 31 modules, with their module and vtable RVA.
- `buttons.json`: 16 button flags (attack, jump, duck, etc.).

### What the dumper does NOT give you (must be found by hand)
1. **The bone array pointer.** Not in the schema. Valve keeps it out. Find it by:
   - **Cheat Engine:** get a pawn, follow `pawn + m_pGameSceneNode`, walk forward into the model state, look for a
     pointer to a block of `matrix3x4a_t` values whose first entry tracks the pawn's world position as the pawn
     moves. Once found, record the byte offset in `docs/offsets.md` with the build number.
   - **Pattern scan:** find the function in `client.dll` that computes bone matrices or reads the array, and read
     the offset from the instruction.
   For Phase 4, skip bones entirely. Add them later. A box ESP with name, health, team, distance is a working ESP.
2. **`CGlobalVars` internal layout.** Standard Source 2 layout; verify against `dwGlobalVars` on this build.
   Typical: `0x00 realtime`, `0x04 framecount`, `0x10 curtime`, `0x18 maxClients`, `0x1C interval_per_tick`.
3. **Entity list iteration logic.** Chunks are 512 entries each; `entity_list + 0x10 + (index >> 9) * 0x8` is the
   chunk pointer; `chunk + (index & 0x1FF) * 0x78` is the entity identity; `identity + 0x0` is the entity pointer;
   `identity + 0x10` is the serial. This is code, not offsets.

### Interfaces from outside the process (`game/interfaces.h`, Phase 2)
- `CreateInterface` is exported by `tier0.dll` and `client.dll`. Resolve it from the remote process by parsing
  the remote PE's export directory, or by walking the `InterfaceReg` linked list.
- `CreateInterface("GameEntitySystem001", nullptr)` returns the entity system pointer, read remotely.
- `CreateInterface("SchemaSystem_001", nullptr)` returns the schema system pointer.
- Log every interface pointer at startup; if one is null, disable the features that depend on it.

### Handle resolution (`game/handle.h`, Phase 3)
Every `m_h*` field (weapons, pawns, observers, defusers) is a `CHandle` (uint32): low 15 bits are the index, high
bits are the serial. Resolution:
```cpp
index  = handle & 0x7FFF;
serial = handle >> 15;
identity = read(entity_list + 0x10 + (index >> 9) * 0x8);
entry    = read(identity + (index & 0x1FF) * 0x78);
entity   = read(entry + 0x0);
if (read<uint32>(entry + 0x10) != serial) return nullptr; // stale
```

### Struct reconstruction approach
Model only what we use. Unknown bytes are explicit padding, and **every** field is either resolved from
`client_dll.json` or pinned with a `static_assert`:

```cpp
// game/structs.h
struct Vec3 { float x, y, z; };

// Accessor helpers rather than direct struct member access, because the field offsets
// are runtime values from the dump, not compile-time constants.
namespace game::fields {
    std::int32_t health(std::uintptr_t pawn) {
        return safe_read<std::int32_t>(pawn + schema::base_entity::m_iHealth);
    }
    // ...
}
static_assert(sizeof(void*) == 8, "x64 only");
```
Rules:
- Never construct, copy-assign into, or `delete` a game object. Read through a validated pointer, copy into a
  `PlayerSnapshot`, write single fields back.
- Verify each schema offset once at startup; log it.
- The combined offset for a weapon's `m_iItemDefinitionIndex` from the weapon pointer is 0x14FA (via
  `m_AttributeManager` → `m_Item` → `m_iItemDefinitionIndex`), but keep the chain explicit in code so it's
  re-verifiable.

### Validity checks
- **Pointer sanity:** non-null, in user space (`0x10000 ≤ p < 0x7FFFFFFFFFFF`), 8-byte aligned where required.
- **Local player:** valid = valid controller pointer, valid pawn handle, finite position, sane life state.
- **Players:** valid controller + pawn, alive check (`m_lifeState == 0`), team check.
- **Entity list:** clamp iteration count; skip nulls, non-players, invalid handles, dormant entities
  (`m_bDormant`).
- All reads go through `core::safe_read<T>`, which is `__try/__except`-guarded. A bad read returns false, never
  crashes our process.
- **The view matrix can be all zeros** before the first frame of a match: check for sanity before projecting.

### Source 2 `Vec3` and angles
- `Vec3 { float x, y, z; }` — 12 bytes.
- The angle convention is **not AC's**. Verify yaw 0 direction, pitch sign, and yaw range in-game with a
  diagnostic before writing any aim math.
- The view matrix is **4x4 floats, row-major** in Source 2 (unlike OpenGL's column-major). Confirm in-game:
  `clip.x = m[0]x + m[1]y + m[2]z + m[3]`.

---

## 8. Current behaviour (what exists now)

*(Filled in as phases complete. Starts empty.)*

---

## 9. Coding conventions

- C++20, standard library first. RAII for every handle and resource.
- Naming: namespaces `lower_case`; types `PascalCase`; functions and variables `snake_case`; constants `kPascalCase`;
  private members `trailing_underscore_`. Files `snake_case.h/.cpp`. `#pragma once`.
- Includes are relative to `src/external`: `#include "core/log.h"`. Standard headers before project headers.
- Fixed-width integers (`std::int32_t`, `std::uintptr_t`) for anything that mirrors game memory.
- **x64 pointers are 8 bytes.** Use `std::uintptr_t`, never `std::uint32_t`, for addresses.
  `static_assert(sizeof(void*) == 8)`.
- Allman braces, 4-space indent, ~120 columns.
- No magic numbers: offsets and signatures in `game/offsets.h` and `game/schema.h`, tunables/ranges in `config.h`.
- Logging with `logger::info/warn/error` only. Hot-path logging must be rate-limited.
- Simple, readable code over clever code. No macros where a `constexpr`/template/function works.
- Tests mirror the source layout.

---

## 10. Known gotchas

### External specifics
- **`ReadProcessMemory` fails silently on bad addresses.** Always check the return value and the bytes-read count.
  Wrap every read in `core::safe_read<T>` and handle the failure path.
- **Read in chunks.** Reading a large entity struct with one `RPM` call is faster than reading it field by field.
- **`RPM` is slow relative to in-process reads.** A few hundred reads per frame is fine; tens of thousands is not.
  Cache aggressively. Re-read only what changed.
- **Pointer chains are the classic external pattern:** read a base pointer, add an offset, read the next pointer,
  add another offset. Each level is a separate `RPM`. Keep the chains short.
- **The handle must have `PROCESS_VM_READ`.** For write features, add `PROCESS_VM_WRITE` and `PROCESS_VM_OPERATION`.
  Open with the minimum you need.
- **Run as administrator** if `OpenProcess` fails on `cs2.exe` (it will on some setups, because the game runs
  elevated).
- **The game may protect itself against external memory reads** in some configurations. With `-insecure`, it does
  not.
- **Don't call `ReadProcessMemory` on a page that might be freed.** A crash in the target is not your fault, but
  a crash in your process is. `safe_read` handles this.

### CS2 / Source 2 specifics
- **Offsets change on every update.** Subscribe to `a2x/cs2-dumper`. The moment a patch lands, the old offsets are
  wrong.
- **Schema system is your friend.** Field offsets inside entities should be resolved from the dump at runtime, not
  hardcoded ad-hoc. The dump files in `docs/dumps/` are the source of truth until the next patch.
- **`-insecure` disables VAC but not all integrity checks.** Do not join any online server with the tool running.
- **`client.dll` and `engine2.dll`** are both relevant. Load both module bases at startup.
- **Entity list is chunked**, not a flat array. Do not try to index it as `entities[i]` from a base pointer.
- **Players are two entities (controller + pawn).** Read the controller for name/team/money; read the pawn for
  health/position/bones/weapon.
- **`CHandle` is 32-bit:** low bits are the index, high bits are the serial. Validate before dereferencing.
- **The view matrix is row-major** in Source 2. Verify on this build.
- **Bone matrix layout** may be `matrix3x4a_t` or `CTransform`. Verify per update.
- **The game is not single-threaded.** You are reading its memory while it runs. A bot could be spawning or dying
  mid-read. `safe_read` and validity checks handle this; do not assume a snapshot is internally consistent across
  two separate reads (re-read the whole struct if you need consistency).
- **Never call a game function.** External has no way to do this safely. Features that need a game-thread call
  (silent aim, TraceLine) are out of scope for this project.

### Overlay window
- **The overlay is a separate topmost window.** It does not hook the game's Present, does not touch the game's
  swap chain, and does not need to know the game's DX11 device.
- **Click-through:** use `WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOPMOST` and toggle `WS_EX_TRANSPARENT` when
  the menu is open vs closed.
- **The overlay must track the game window's position and size.** Get the game's HWND (find `cs2.exe`'s main
  window), and on every frame, read its client rect and move the overlay to match.
- **DPI awareness:** make the overlay DPI-aware (`SetProcessDpiAwarenessContext` or the manifest) so the overlay
  coordinates match the game's client coordinates.
- **The overlay can be captured by screenshots and OBS.** If that matters, don't use the overlay for anything you
  don't want visible. (Offline only, so it shouldn't.)
- **Alt-tab behaviour:** the overlay should stay topmost over the game but not over other windows. Handle
  `WM_ACTIVATE` to hide/show.

### Tests (doctest)
- **Define `DOCTEST_CONFIG_USE_STD_HEADERS`** (set in `tests.vcxproj`).
- doctest is on the **external** include path with external warnings off.

---

## 11. Dos and don'ts

**Do**
- Respect §1. If a task drifts toward online use, evasion, stealth, networking or distribution, stop and tell the
  user.
- Open the process handle with the minimum access rights needed.
- Wrap every game read in `core::safe_read<T>` and every write in `core::safe_write<T>`. Never a raw `RPM` call
  outside `core/memory`.
- Keep maths and feature logic pure (no Win32, no ImGui, no game pointers) and unit-test it.
- Keep commits small and focused; commit only after the user approves.
- Build Debug and Release with **zero warnings** before calling a task done.
- Restore anything you change in the game on panic and/or shutdown.
- Ask the user before new dependencies or big architectural changes.
- **Record every offset, signature, and schema field in `docs/offsets.md`, with its source (repo + commit + date)
  and proof.**

**Don't**
- Don't inject, hook, or call into the game. External means external.
- Don't touch game state from any thread other than the main thread.
- Don't let exceptions escape (there is no game code to escape into, but be disciplined about it).
- Don't change a verified offset without a diagnostic proving it.
- Don't hardcode offsets or signatures outside `game/offsets.h` and `game/schema.h`.
- Don't write to the game's code. No patches. That's an internal-only technique.
- Don't add DMA, manual mapping, module/thread hiding, obfuscation, anti-debug, network code or packaging.
- Don't edit vendored code in `external/`.
- Don't add Claude/AI co-author or attribution lines to commits or PRs.
- **Don't ever join a VAC-secured server with the tool running.** `-insecure` only. Offline bots only.

---

## 12. Testing

- **Unit tests** (from Phase 1): `tests` console exe (x64, doctest v2.5.3) that compiles the **pure** sources from
  `src/external` (plus `core/memory.cpp` and `core/pattern.cpp`, which are Win32 but harmless to test) and
  `tests/**/*.cpp`. Run: `bin\Debug\tests.exe` (and `bin\Release\tests.exe`). When adding a pure `.cpp` under
  test, add it to `tests.vcxproj` too.
- **In-game tests:** every phase lists acceptance criteria in §13. The user runs them; record the result in §14 and
  DEVLOG.
- **Offset re-verification test** (after every CS2 update): run the tool, check the console for signature hits,
  interface pointers, and schema field offsets. Anything null or mismatched is a flag. Cross-check against a fresh
  `a2x/cs2-dumper` run.

---

## 13. Roadmap

Each phase ends with: zero-warning Debug + Release builds, tests passing (once they exist), the user's in-game
check, docs updated (§2 step 7), then a commit after approval.

### Phase 0: Scaffold + process handle
- [ ] Solution, `external` (console exe) and `tests` projects, shared `props/common.props` (C++20, x64, `/MT`,
      `/W4 /WX`)
- [ ] `.gitignore`, `.gitattributes`, README stub, `docs/DEVLOG.md`, `docs/offsets.md`, `docs/dumps/`,
      `external/README.md`, this file
- [ ] `core/process`: find `cs2.exe` (Toolhelp32), open a handle, look up `client.dll` and `engine2.dll` module
      bases
- [ ] `core/memory`: `safe_read<T>` / `safe_write<T>` wrappers around `ReadProcessMemory` / `WriteProcessMemory`,
      with `__try/__except` guards and an `is_plausible_pointer` helper
- [ ] `core/log`: `logger::info/warn/error`
- [ ] `main.cpp`: open the handle, print the module bases and the local pawn pointer, wait for Enter, exit
- [ ] Builds with zero warnings (Debug + Release)
- [ ] Verified in-game by the user (launch with `-insecure`, offline bots)

**Acceptance:** the tool finds `cs2.exe`, prints the PID, the `client.dll` base, the `engine2.dll` base, and the
local pawn pointer (plausible hex). It exits cleanly. Running it with the game closed says "cs2.exe not found."
Running it without admin, if the game is elevated, says "OpenProcess failed" and tells the user to run as admin.

### Phase 1: Overlay window + ImGui shell
- [ ] Vendor Dear ImGui (pinned tag) + Win32 + DX11 backends
- [ ] `ui/overlay_window`: transparent, click-through, topmost Win32 window with its own DX11 swap chain; tracks
      the game window's client rect; toggles `WS_EX_TRANSPARENT` on INSERT
- [ ] `ui/imgui_layer`: ImGui context + backends for the overlay window; shutdown on exit
- [ ] `ui/theme` (dark), `ui/menu` with sidebar pages (ESP / Aimbot / Triggerbot / Player / Misc / Settings,
      placeholders), branding "Internal Cheat - by BigH (External)"
- [ ] INSERT toggles the menu (temporary hard-coded until Phase 7)
- [ ] Builds with zero warnings (Debug + Release)
- [ ] Verified in-game by the user

**Acceptance:** the overlay appears on top of CS2, sits over the game window exactly, and is click-through when
the menu is closed. INSERT opens the menu; the cursor is visible and clicks work while the menu is open; clicking
through the menu doesn't shoot the game. Alt-tab hides the overlay when the game loses focus. Closing the tool
removes the overlay and leaves the game running.

### Phase 2: Offsets, signatures, interfaces, schema
- [ ] Copy `dw*` offsets from `docs/dumps/offsets.json` into `offsets.h`, with provenance
- [ ] Copy the schema fields we need from `docs/dumps/client_dll.json` into `schema.h`, with provenance
- [ ] `core/pattern`: remote signature scanner (read the module range in chunks, scan locally), tested
- [ ] `game/interfaces`: resolve `CreateInterface` from outside the process (parse the remote PE export directory,
      or walk the `InterfaceReg` linked list); look up `GameEntitySystem`, `SchemaSystem`, `Source2Client`,
      `EngineClient`, `InputSystem`
- [ ] A diagnostic that prints every signature hit, every interface pointer, and every schema field offset
- [ ] Builds with zero warnings (Debug + Release); tests pass
- [ ] Verified in-game by the user

**Acceptance:** the console shows non-null interface pointers, every schema field offset (health, team, life
state, origin, view offset, eye angles, weapon services, active weapon, shots fired, dormant), and every signature
address. Every value matches what a fresh `a2x/cs2-dumper` run of the current CS2 build shows, or the diagnostic
explains the mismatch.

### Phase 3: Entity list + local player + snapshots
- [ ] `game/entities`: walk the chunked entity list remotely; yield entities; filter to `C_CSPlayerController`
- [ ] `game/handle`: `CHandle` → entity pointer resolution, with serial validation
- [ ] `game/player`: build `PlayerSnapshot` from a controller (resolve pawn via `m_hPlayerPawn`); read health,
      team, position, life state, name, weapon id; validity checks
- [ ] `game/view`: read the view matrix; sanity check
- [ ] `game/globals`: read `CGlobalVars` (curtime, maxClients, interval_per_tick)
- [ ] `game/weapon`: read the active weapon id; map to a name
- [ ] `app/live_view`: debug console view (~4 Hz) with local player + every bot
- [ ] `maths/vec`, `game/snapshot.h` (pure), tested
- [ ] Builds with zero warnings (Debug + Release); tests pass
- [ ] Verified in-game by the user

**Acceptance:** live view shows the correct name, team, health, life state, weapon, and position for you and
every bot in a bot match; dying and respawning updates correctly; a bot match restart doesn't crash; the view
matrix is sane before and during a frame.

### Phase 4: World-to-screen + ESP
- [ ] `maths/projection`: `ViewMatrix`, `world_to_screen` (row-major, matching this build's matrix); tests
- [ ] `maths/skeleton`: joint specs, bone index → screen positions (only if bones are found)
- [ ] `features/esp`: pure decision; `render/primitives` + `render/painter` on the overlay's ImGui background
      draw list
- [ ] ESP page: 2D box, corner box, head circle, skeleton (optional), name, health bar/number, distance, weapon
      name, snaplines (origin choice), team mode, enemies only, colours with opacity, thickness
- [ ] Builds with zero warnings (Debug + Release); tests pass
- [ ] Verified in-game by the user

**Acceptance:** boxes line up with bots at near/far distance and different resolutions; nothing drawn for bots
behind you; skeleton faces the right way (if bones are done); colours/opacity change live; dead bots not drawn;
map change doesn't crash; tests pass (including the row-major matrix centre test).

### Phase 5: Aimbot + triggerbot
- [ ] `maths/angles`: `Angles`, `normalize`, `clamp`, `calc_aim_angles`, `angular_distance`, `is_within_fov`,
      `smoothing_fraction`, `step_towards`; tests
- [ ] `features/aimbot`: candidates (FOV, dead, team, max distance), priority (crosshair / distance / lowest
      health), head/body/nearest-bone, compute aim; tests
- [ ] `features/triggerbot`: state machine with reaction delay; tests
- [ ] Angle write via `WriteProcessMemory` on the main thread; no aim while the menu is open or the game isn't
      focused
- [ ] Aimbot page + Triggerbot page: enable, activation key (hold/toggle), aim point, priority, FOV radius +
      circle, smoothing, team check, max distance, triggerbot reaction delay and hold key
- [ ] Builds with zero warnings (Debug + Release); tests pass
- [ ] Verified in-game by the user

**Acceptance:** holding the aim key locks on the right target for each priority; head/body/nearest-bone works;
smoothing feels consistent at different framerates; FOV circle matches which bots are eligible; team check and
max distance filter correctly; triggerbot fires only with an enemy under the crosshair; neither feature fires
while the menu is open or the game isn't focused; tests pass.

**Honest note:** external aimbot cannot write angles on the game's logic thread, so the view will feel slightly
laggier than an internal one. That's the cost of external. It's still usable for offline learning.

### Phase 6: Player values
- [ ] `settings/settings.h` (player section)
- [ ] `features/player_values` (pure, tested)
- [ ] `game/` field writes by id (health, armour, ammo per weapon)
- [ ] Player page: target, live "Now", Set, Freeze per value
- [ ] Builds with zero warnings (Debug + Release); tests pass
- [ ] Verified in-game by the user

**Acceptance:** Set health 999 shows on the HUD; frozen health stays put; per-weapon ammo works; no writes while
dead; exit stops all writes and the game continues normally; tests pass.

### Phase 7: Keybind engine
- [ ] `input/keys`, `input/actions` (registry + defaults), `input/keybinds` (HOLD/TOGGLE/PRESS), tested
- [ ] `input/bind_capture` (pure, tested)
- [ ] `input/key_poll` on the main thread; act only while the game is focused; suspended during capture
- [ ] Keybinds page: every action by category, click to capture, mode selector, conflict highlight; bind buttons
      on each page
- [ ] Actions: menu toggle (INSERT), panic, exit, aimbot (HOLD/TOGGLE), aimbot enable, ESP toggle, triggerbot
      (HOLD/TOGGLE), set/freeze per value, presets
- [ ] Builds with zero warnings (Debug + Release); tests pass
- [ ] Verified in-game by the user

**Acceptance:** each mode works with keyboard keys and mouse buttons (incl. MOUSE4/5); capturing a bind doesn't
fire it; conflicts are shown; rebinding the menu key works; tests pass.

### Phase 8: Settings and profiles (JSON) + presets
- [ ] Vendor nlohmann/json (after approval)
- [ ] `settings/profile_json`: one field list per section for write + read; forgiving load, `schema_version` +
      migration hook (pure, tested)
- [ ] `settings/profile_store`: `ProfileStore(folder)`, atomic save, read-only `default`, last profile, load on
      startup (std::filesystem only, tested on a temp folder)
- [ ] Settings page: profile list, save / save as / load / rename / delete / reset, unsaved-changes marker
- [ ] `settings/presets`: Off / Chill / Medium / Rage, feature bundles + strengths, on the Settings page and
      bindable hotkeys
- [ ] `profiles/default.json` committed and equal to code defaults (test)
- [ ] Builds with zero warnings (Debug + Release); tests pass
- [ ] Verified in-game by the user

**Acceptance:** settings survive restart via the last profile; a hand-edited profile with bad values loads with
warnings and clamped values; `default` can't be overwritten; each preset switches the right features and leaves
keybinds, colours and Player values alone; tests pass.

### Phase 9: Panic, clean shutdown, polish
- [ ] Panic: disables aimbot/ESP/triggerbot and every write feature, unfreezes everything, closes the menu
- [ ] Exit action + Settings-page button: restore everything, shut down the overlay, close the handle, exit
- [ ] Final key layout: **DELETE = exit, END = panic** (INSERT menu)
- [ ] Robustness review: map change, death, alt-tab, minimise, game close, game restart
- [ ] Builds with zero warnings (Debug + Release); tests pass
- [ ] Verified in-game by the user

**Acceptance:** panic returns the game to normal instantly; 10 start/stop cycles with no crash, leak, input or
cursor problem; after exit the game behaves exactly like an untouched game.

### Phase 10: UI redesign
- [ ] Rename to the final name (decided with the user)
- [ ] Logo embedded (menu header + Home via an ImGui user texture)
- [ ] Home dashboard + grouped sidebar with icons (drawn with the draw list, no icon font)
- [ ] Colours from the logo, (?) help tooltips, two-column pages, menu size setting
- [ ] Builds with zero warnings (Debug + Release); tests pass
- [ ] Verified in-game by the user

### Phase 11: Educational README
- [ ] A comprehensive, educational `README.md`: what the project is (and isn't: §1), features, how it works
      (process handle, RPM/WPM, entity system walk, handle resolution, schema lookups, signature scanning,
      world-to-screen, overlay window), how offsets and signatures were found (the dumpers, Cheat Engine,
      IDA/Ghidra), safety (guarded reads, shutdown), build and use (keys, presets, profiles), project layout,
      tests, and what was learned per phase
- [ ] Final pass on this file
- [ ] Builds with zero warnings (Debug + Release); tests pass
- [ ] Reviewed by the user

**Acceptance:** someone who has never seen the project can understand what it does, how each technique works and
why, build it and use it, from the README alone.

### Phase 12+ (only if the user decides to): Internal
- [ ] This is a **separate, explicitly-scoped decision**, not a silent slide. If the user wants silent aim, no
      spread, no recoil, or sub-tick angle writes, that requires a DLL, injection, and hooks. Re-read §1, re-scope
      the project, and start a new `CLAUDE.md` section for the internal side. Do not bolt it onto the external
      codebase.

### Recurring: after every CS2 update
- [ ] Re-run `a2x/cs2-dumper` (or pull the latest from a mirror)
- [ ] Diff the new `output/offsets.json` against `docs/dumps/offsets.json`; copy the new values into `offsets.h`
      and log the change in `docs/offsets.md`
- [ ] Diff the new `output/client_dll.json` against `docs/dumps/client_dll.json`; copy the new field offsets into
      `schema.h` and log any that changed
- [ ] Every signature that no longer hits: find the new address, update `offsets.h`, log it
- [ ] Rebuild, re-run tests, re-run the tool, spot-check ESP/aimbot/triggerbot/values

---

## 14. Current status

- **Phase 0: not started.**
- **Prerequisites done:** a2x/cs2-dumper has been run. `docs/dumps/offsets.json` and `docs/dumps/client_dll.json`
  exist and look healthy (29 offsets across 5 modules, 3301 classes, 569 enums; two non-critical pattern misses
  on `dwSensitivity` and `dwSoundSystem_engineViewData` are cosmetic).

**Next:** Phase 0 — scaffold the solution, projects, and the process handle.

---

## 15. Decision log

- **2026-10-06:** Project started as an **external-first** offline-only CS2 trainer for learning. `-insecure` only.
  No VAC servers. Private repo. The user is the sole author (no AI attribution in commits or PRs).
- **2026-10-06:** **External-first, not internal.** The reason: external teaches the fundamentals (pointer chains,
  entity system, schema lookups, world-to-screen) without the crash risk, dev-loop friction, and injection
  complexity of a DLL. Internal features (silent aim, no spread, no recoil, sub-tick angle writes) are explicitly
  deferred to a separate Phase 12+ decision, not bundled in.
- **2026-10-06:** x64 only. No x86 configuration. CS2 is 64-bit.
- **2026-10-06:** Use the community offset dumpers (`a2x/cs2-dumper`, `sezzyaep/CS2-OFFSETS`,
  `xsip/cs2-sdk-offsets-interfaces-dump`, `scros22/cs2-universal-offsets`) as the source of truth for stable RVAs
  and schema fields. Record provenance (repo + commit + date) next to every offset in `offsets.h` and `schema.h`.
- **2026-10-06:** Use **schema system** for field offsets inside entities rather than hardcoding them where the
  remote walk is viable; fall back to a dumped `client_dll.json` with provenance if the walk is too fragile.
- **2026-10-06:** The overlay is a **separate topmost window**, not a hook of the game's Present. This is the
  defining architectural difference from an internal overlay.
- **2026-10-06:** `docs/offsets.md` is a first-class artifact, not an afterthought. Every offset, every signature,
  every schema field, with its source and its proof.
- **2026-10-06:** The bone array pointer is **not** in the schema dump. It must be found by hand (Cheat Engine or
  pattern scan) and recorded in `docs/offsets.md` with the build number. Skip bones in Phase 4; add them later.

---

## 16. Maintenance rule

**After EVERY task**, before finishing:
1. Update the **file tree** (§5): status markers, new/removed files, one-line descriptions.
2. Tick the **roadmap** checkboxes (§13) and update **Current status** (§14) and its **Next** line.
3. Update `docs/offsets.md` with every offset, signature, and schema field found this session, plus its source
   (repo + commit + date) and how it was proven.
4. Update any rules, gotchas or patterns that changed (§4–§12) and add to the **Decision log** (§15).
5. Add a dated entry to `docs/DEVLOG.md` (what was built, bugs found and how they were fixed).