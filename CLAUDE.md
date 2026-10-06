# CLAUDE.md: External Cheat (by BigH), CS2 Offline Learning Trainer (External-First)

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
      **If `cs2_external.exe` is running** (it locks `bin\<Config>\cs2_external.exe`, LNK1104), **force-close it
      and build**: `Get-Process cs2_external -ErrorAction SilentlyContinue | Stop-Process -Force`. Standing
      permission from the user (2026-10-06); don't ask, don't build into a scratch folder instead. Only our tool,
      never `cs2.exe`. Say in the summary that it was closed.
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

**External Cheat - by BigH (CS2 Offline)** (the name, set by the user 2026-10-06; it replaced the AC project's
"Internal Cheat"): an external trainer for CS2 with an in-game
overlay menu, running **only** under `-insecure` against bots.

The goal is a learning project, not parity with any public cheat and not a release. Feature targets, in order of
implementation:

- **Overlay menu** toggled with **INSERT**: a Home dashboard and sidebar pages (**ESP, Aimbot, Triggerbot, Misc,
  Settings**). Help in (?) tooltips, adjustable menu size, settings applied live.
- **ESP** drawn in the overlay: 2D box, corner box, head circle, approximate skeleton (Phase 4+, needs bones),
  name, health bar/number, distance, weapon name, **scoped indicator** (the bot is zoomed in with an AWP, SSG 08,
  SCAR-20, G3SG1, AUG or SG 553), **visible / hidden colours** (spotted-by heuristic), snaplines (origin choice),
  team mode, enemies-only, colour picker with opacity.
- **Aimbot:** hold/toggle key, head/body target, priority (crosshair / distance / lowest health), FOV radius +
  circle, smoothing, team check, max distance, **visible only** (spotted-by heuristic).
- **Triggerbot** (fully configurable): fires automatically with a living enemy under the crosshair. Activation
  always on / hold key / toggle key; reaction delay before the first shot; fire mode single tap / burst (N shots) /
  hold while on target; delay between shots; team check; visible only; max distance; weapon-class filter (pistol,
  SMG, rifle, sniper, shotgun, heavy); snipers only when scoped; don't fire while flashed or in the air; head only
  (after bones are found).
- **Bunny hop** (Misc): hold space and the tool times each jump to the moment you land.
- **Watermark** on the game: the logo, "External Cheat by BigH", and under it the features that are on, one per
  line. No menu-key hint.
- ~~Player values (health / armour / ammo writes)~~: **dropped** by the user (2026-10-06). No Player page, no
  player-value writes.
- **Radar / misc** (with bunny hop, Phase 6): enlarged radar, enemy dots on radar, bomb timer, spectator list,
  hitsound (via the overlay's own audio, not the game's).
- **Keybinds** for every action, **profiles** saved as JSON, **panic key**, clean shutdown.
- **Presets** (Off / Chill / Medium / Rage) that switch features and strengths.

**What external cannot do, and we accept that:**
- No silent aim (that requires a hook inside `CreateMove`).
- No weapon code patches (no spread, no recoil via patching).
- No calling game functions (TraceLine, etc.). An exact "is this bot visible right now" check is a game trace,
  which only internal code can call. External uses the **spotted-by heuristic** instead (see the feasibility table
  below); an own ray cast against the map's collision mesh, parsed from the game files, would be external too but is
  a big separate project, not planned.
- No sub-tick angle writes.
- If the user ever wants those, **that's a separate Phase 12+ decision** with a full re-scope, not a silent slide
  into internal.

**Feasibility check of the features the user asked for on 2026-10-06** (all external, nothing internal needed):

| Feature | External? | How (fields from build 14189's dump, re-verified in Phase 2) |
|---|---|---|
| Bunny hop | Yes | Each frame while space is held: read the local pawn's `m_fFlags` (`C_BaseEntity` `0x3F4`, bit 0 = `FL_ONGROUND`). On landing, press jump by writing the `jump` button state (`buttons.json`, client.dll `+0x22324E0`); fallback `SendInput`. Needs a write handle. Timing depends on our loop rate vs the 64-tick server: a vsync-limited loop can miss hops, so Phase 6 may need a faster polling path |
| ESP: scoped indicator | Yes | One byte per bot: `C_CSPlayerPawn::m_bIsScoped` (`0x1EA0`); the weapon name (Phase 3) says which gun |
| Visibility check (aimbot, ESP, triggerbot) | **Heuristic only** | Exact line of sight = the game's trace = internal. External: `C_CSPlayerPawn::m_entitySpottedState` (`0x1E88`) → `EntitySpottedState_t::m_bSpottedByMask` (`+0xC`), a bit per player slot that the game itself sets when that player has line of sight (it feeds the radar). In an offline match our own client runs the server, so it's kept up to date, but it lags a little and is a "spotted" flag, not a per-frame ray |
| Triggerbot (configurable) | Yes | Crosshair target: local `C_CSPlayerPawn::m_iIDEntIndex` (`0x36CC`, the entity under the crosshair). Fire by writing the `attack` button state (`buttons.json` `+0x2231FD0`) or `SendInput`. Flash: `C_CSPlayerPawnBase::m_flFlashDuration` (`0x1510`). Air: `m_fFlags`. Scoped: local `m_bIsScoped`. Head only needs the bone array (§7) |

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
- **Import order matters:** `common.props` is imported in the `PropertySheets` ImportGroup **after**
  `Microsoft.Cpp.props` (otherwise the toolset defaults override it). `WindowsTargetPlatformVersion` and
  `CharacterSet` must be set **before** `Microsoft.Cpp.props`, so those two (and only those two) live in each
  `.vcxproj`. Copy that layout for every new project.
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
- **a2x/cs2-dumper** run successfully. Full output (every module, in .cs/.hpp/.json/.rs/.zig) is at
  `C:\Users\Harry\Desktop\output`; only the JSON files we use are copied into `docs/dumps/`. Copy, never retype.
- **Python 3 + Pillow** (optional): only to regenerate `src/external/ui/logo_pixels.h` after changing
  `assets/logo.jpg` (`python tools/make_logo_header.py`). The generated header is committed; builds don't need Python.
- **CS2 test setup:** an offline Deathmatch against bots with the bots frozen (they can't shoot), round time 60 min.

---

## 5. File tree

Status markers: ✅ exists, 🔲 planned (phase number in brackets).

```
cs2-external/
  CLAUDE.md                       ✅ this file: project memory and rules
  README.md                       ✅ stub (full educational README in [11])
  .gitignore                      ✅ VS/C++ build output, .vs, local profiles/logs
  .gitattributes                  ✅ CRLF for VS files, text normalisation
  cs2-external.sln                ✅ solution: Debug|x64, Release|x64 (external, tests, vendor)
  assets/
    logo.jpg                      ✅ the logo (README, and the source of ui/logo_pixels.h)
    logo.ico                      ✅ the exe's icon (32x32)
  tools/
    make_logo_header.py           ✅ assets/logo.jpg → src/external/ui/logo_pixels.h (128x128 RGBA, needs Pillow)
  props/
    common.props                  ✅ shared build settings (C++20, /W4 /WX, /MT, output dirs); import AFTER Cpp.props
    imgui.props                   ✅ ImGui config defines + external include paths (vendor + external import it)
  docs/
    DEVLOG.md                     ✅ dated log
    offsets.md                    ✅ running notes on every offset, signature, schema field, with proof
    dumps/
      info.json                   ✅ a2x/cs2-dumper run info: CS2 build 14189, 2026-10-06T16:04:45Z
      offsets.json                ✅ a2x/cs2-dumper output (module globals, 29 entries)
      client_dll.json             ✅ a2x/cs2-dumper output (542 schema classes, 14 enums)
      interfaces.json             ✅ a2x/cs2-dumper output (111 interfaces across 31 modules)
      buttons.json                ✅ a2x/cs2-dumper output (16 button globals)
  external/
    README.md                     ✅ what's vendored, versions, licences, blob hashes
    vendor.vcxproj                ✅ static lib (vendor.lib) compiling Dear ImGui, warnings off
    imgui/                        ✅ Dear ImGui v1.92.9b core + Win32 + DX11 backends (15 files, hashes in README)
    doctest/                      ✅ doctest v2.5.3 (doctest.h + LICENSE.txt)
    nlohmann/                     🔲 [8] nlohmann/json v3.12.0
  profiles/
    default.json                  🔲 [8] mirror of built-in defaults
  src/
    external/
      external.vcxproj            ✅ console exe → bin\<Config>\cs2_external.exe, x64, static CRT
      external.rc                 ✅ the exe icon (assets/logo.ico)
      main.cpp                    ✅ bootstrap: DPI awareness, console Ctrl handler, find cs2.exe → read-only handle
                                     → print PID, module bases, local pawn → app::run
      core/
        process.h/.cpp            ✅ UniqueHandle, find_process, open_handle, module_base (Toolhelp32),
                                     find_main_window (EnumWindows), is_running
        memory.h                  ✅ PURE: is_plausible_pointer/range + core::Memory interface (read_bytes,
                                     safe_read<T>, read<T>, write_bytes, safe_write<T>)
        process_memory.h/.cpp     ✅ ProcessMemory : Memory, the only RPM/WPM calls (__try/__except-guarded)
        pattern.h/.cpp            🔲 [2] signature scanner over a remote module
        log.h                     ✅ logger::info/warn/error (std::format, stdout)
        runtime.h                 ✅ shutdown_requested / shutdown_complete (set by the console Ctrl handler)
      game/                       (THE ONLY place that dereferences game memory)
        offsets.h                 ✅ module RVAs (from dumps/offsets.json); only dwLocalPlayerPawn so far
        schema.h                  🔲 [2] class field offsets (from dumps/client_dll.json)
        interfaces.h/.cpp         🔲 [2] CreateInterface resolution from outside the process
        structs.h                 🔲 [3] Vec3, PlayerSnapshot, accessor helpers
        player.h/.cpp             ✅ read_local_pawn (Phase 0); [3] make_snapshot, validity checks, collect_players
        entities.h/.cpp           🔲 [3] walk the chunked entity list remotely
        handle.h/.cpp             🔲 [3] CHandle → entity pointer resolution
        view.h/.cpp               🔲 [3] view matrix read
        bones.h/.cpp              🔲 [4+] bone array read (needs a self-found offset, see §7)
        weapon.h/.cpp             🔲 [3] read weapon id / name
        globals.h/.cpp            🔲 [3] CGlobalVars reads (curtime, maxClients, interval_per_tick)
      maths/                      (PURE)
        vec.h                     🔲 [3] Vec2, Vec3 + ops
        angles.h/.cpp             🔲 [5] Angles, normalize, calc_aim_angles, angular_distance, smoothing
        projection.h/.cpp         🔲 [4] ViewMatrix, world_to_screen (row-major, matching this build)
        skeleton.h/.cpp           🔲 [4+] joint specs, bone index → screen positions
      features/                   (PURE: data in, decisions out)
        esp.h/.cpp                🔲 [4] health_colour, screen_box, display_name, build_esp → primitives
        aimbot.h/.cpp             🔲 [5] aim_point, find_candidates, select_target, compute_aim
        triggerbot.h/.cpp         🔲 [5] Triggerbot state machine
        feature_summary.h/.cpp    ✅ ActiveFeatures + feature_summary() → "ESP · Aimbot" for the watermark
      render/
        primitives.h              🔲 [4] Line, Rect, FilledRect, Circle, Text, Primitive variant (PURE data)
        painter.h/.cpp            🔲 [4] paint(primitives, font) on ImGui background draw list
      input/
        keys.h/.cpp               🔲 [7] VK ↔ names, KeySet
        actions.h/.cpp            🔲 [7] ActionId registry
        keybinds.h/.cpp           🔲 [7] HOLD/TOGGLE/PRESS engine
        key_poll.h/.cpp           🔲 [7] GetAsyncKeyState polling
        bind_capture.h/.cpp       🔲 [7] bind capture
      settings/
        settings.h                🔲 [4] Settings struct (ESP section first, aimbot/triggerbot in [5])
        profile_json.h/.cpp       🔲 [8]
        profile_store.h/.cpp      🔲 [8]
        presets.h/.cpp            🔲 [8]
      ui/
        imgui_layer.h/.cpp        ✅ ImGuiLayer: context, fonts, logo texture, Win32 + DX11 backends;
                                     imgui_message_hook
        logo_pixels.h             ✅ GENERATED (tools/make_logo_header.py): the logo as 128x128 RGBA pixels
        overlay_window.h/.cpp     ✅ transparent, click-through, topmost window + own D3D11 device/swap chain; covers the
                                     game's client rect; INSERT hotkey; set_interactive (click-through off/on)
        theme.h/.cpp              ✅ dark navy palette (AC's), apply_theme, scaled()
        widgets.h/.cpp            ✅ page_header, card, hint, help_marker, info_row, notice, pill, planned_card,
                                     image_rounded
        menu.h/.cpp               ✅ header (logo, title, pills) + grouped sidebar + current page
        hud.h/.cpp                ✅ watermark (logo + active features) + frame outline, background draw list
        pages/                    ✅ pages.h + one file per page: home (logo, live status), aimbot, triggerbot, esp,
                                     misc (placeholders), settings (overlay switches)
      color.h                     🔲 [4] Color (RGBA floats)
      config.h                    ✅ PURE: branding, pointer bounds, process/module names, menu key, menu sizes,
                                     timings; [5+] Range<T> + every setting range
      app/                        (the orchestrator, from AC: the only place that wires everything together)
        frame.h/.cpp              ✅ app::run: the loop (pump → game window/focus → menu key → status read → draw →
                                     present) and the overlay's teardown
        state.h                   ✅ AppState (game info, overlay options, active features, menu open, match status,
                                     overlay size/FPS)
        live_view.h/.cpp          🔲 [3] debug console view (~4 Hz) with local player + every bot
  tests/
    tests.vcxproj                 ✅ console exe (doctest) → bin\<Config>\tests.exe
    main.cpp                      ✅ DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
    helpers/fake_memory.h         ✅ FakeMemory : core::Memory (mapped regions at fake addresses, read/write counts)
    core/test_memory.cpp          ✅ pointer checks + Memory's typed wrappers (via FakeMemory)
    core/test_process_memory.cpp  ✅ ProcessMemory RPM/WPM against our own process (uncommitted page, partial read)
    game/test_player.cpp          ✅ read_local_pawn (Phase 0); [3] snapshots
    core/test_pattern.cpp         🔲 [2] pattern parse, wildcard match, scan
    maths/test_angles.cpp         🔲 [5]
    maths/test_projection.cpp     🔲 [4]
    maths/test_skeleton.cpp       🔲 [4+]
    game/test_entities.cpp        🔲 [3] fake remote reader
    game/test_handle.cpp          🔲 [3] CHandle resolution
    features/test_esp.cpp         🔲 [4]
    features/test_aimbot.cpp      🔲 [5]
    features/test_triggerbot.cpp  🔲 [5]
    features/test_feature_summary.cpp ✅ the watermark's feature line (empty, one, order, all)
    settings/test_settings.cpp    🔲 [4]
    settings/test_profile_json.cpp 🔲 [8]
    settings/test_profile_store.cpp 🔲 [8]
    settings/test_presets.cpp     🔲 [8]
    input/test_keys.cpp           🔲 [7]
    input/test_keybinds.cpp       🔲 [7]
    input/test_bind_capture.cpp   🔲 [7]
    test_color.cpp                🔲 [4]
```

---

## 6. Architecture

### 6.1 Module responsibilities and dependency rules
- **Pure modules** (no `<Windows.h>`, no ImGui, no raw game pointers): `maths/`, `features/`,
  `render/primitives.h`, `game/snapshot.h`, `input/keys|actions|keybinds`, `settings/`, `config.h`,
  `core/memory.h` (the interface, not the RPM implementation), `game/offsets.h`,
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
- **`main.cpp`** is only the bootstrap: find the process, open the handle, build `core::ProcessMemory`, hand over to
  `app/`, and handle shutdown. (Phase 0 does its single read straight from `main.cpp`.)
- **`app/frame`** (from Phase 1, same role as in the AC project) is the orchestrator: the only place that wires
  input, settings, game reads, features, writes and drawing together, one frame at a time.

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
              (all reads go through core::Memory; the real one is ProcessMemory, __try/__except-guarded)
  4. features (pure): aimbot → angles (write via WriteProcessMemory) · esp → primitives
  5. apply:   game/ writes
  6. draw:    ImGui NewFrame → render/painter(primitives) on background draw list → ui::menu (if open)
              → ImGui Render → ImGui_ImplDX11_RenderDrawData on the overlay's swap chain
  7. Present the overlay swap chain
```

### 6.4 Shutdown (no unload sequence)
1. User presses the unload key (DELETE) or closes the overlay window.
2. Restore everything we changed in the game (the only write is the aimbot's view angles, which simply stop; nothing
   else is changed
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
- Presets change feature switches and strengths only, never keybinds, colours or general settings.

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
- `client_dll.json`: 542 classes and 14 enums (build 14189), with every field offset. This is where health, team, position,
  eye angles, weapon handle, player name, and everything else lives.
- `interfaces.json`: 111 interfaces across 31 modules, with their module and vtable RVA.
- `buttons.json`: 16 button flags (attack, jump, duck, etc.).

### Fields and buttons the planned features need (from build 14189's dump, copied into code in Phase 2)
| Name | Where | Dump value | Used by |
|---|---|---|---|
| `m_iHealth` | `C_BaseEntity` | `0x34C` | ESP, aimbot, triggerbot |
| `m_fFlags` | `C_BaseEntity` | `0x3F4` | bunny hop (on ground), triggerbot (in air) |
| `m_vecVelocity` | `C_BaseEntity` | `0x430` | bunny hop diagnostics |
| `m_bIsScoped` | `C_CSPlayerPawn` | `0x1EA0` | ESP scoped indicator, triggerbot "only when scoped" |
| `m_entitySpottedState` | `C_CSPlayerPawn` | `0x1E88` | visibility heuristic |
| `m_bSpottedByMask` | `EntitySpottedState_t` | `+0xC` | visibility heuristic (bit per player slot) |
| `m_iIDEntIndex` | `C_CSPlayerPawn` | `0x36CC` | triggerbot (entity under the crosshair) |
| `m_iShotsFired` | `C_CSPlayerPawn` | `0x1EB4` | triggerbot burst counting |
| `m_flFlashDuration` | `C_CSPlayerPawnBase` | `0x1510` | triggerbot "don't fire while flashed" |
| `jump` | `buttons.json`, client.dll | `0x22324E0` | bunny hop |
| `attack` | `buttons.json`, client.dll | `0x2231FD0` | triggerbot |

Writing a button global is a game write: it needs `PROCESS_VM_WRITE | PROCESS_VM_OPERATION`, added in Phase 5 (with
the aimbot's angle writes). The exact value format of a button write is verified in-game before it's used.

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
        return memory.read<std::int32_t>(pawn + schema::base_entity::m_iHealth); // memory: const core::Memory&
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
- All reads go through `core::Memory` (`safe_read<T>` / `read<T>`); the real implementation, `ProcessMemory`, is
  `__try/__except`-guarded. A bad read returns false, never
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

*(Filled in as phases complete.)*

- **Phase 0 (done, verified in-game 2026-10-06):** `cs2_external.exe` finds `cs2.exe` (Toolhelp32), opens it with a
  **read-only** handle (`PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION`), prints the PID, the `client.dll` and
  `engine2.dll` bases and sizes, and the local pawn pointer (`client.dll + dwLocalPlayerPawn`), then waits for Enter.
  Game closed → `cs2.exe not found` (exit 1). Access denied → tells the user to run as administrator. In the main
  menu the pawn prints as `none`; a non-plausible value prints a "dwLocalPlayerPawn may be stale" warning.
- **Phase 1 (built 2026-10-06, in-game check pending):** after the Phase 0 printout the tool keeps running and puts
  its own transparent, topmost, click-through window over CS2's client area (it follows the game window). A watermark
  sits in the top-left corner: logo + "External Cheat by BigH", and under it the active features one per line
  (`No features on` until Phase 4). INSERT opens the menu (header: logo, name, an "External" pill; sidebar: Home / Aimbot / Triggerbot / ESP / Misc /
  Settings): the overlay takes focus so the game lets go of the mouse. INSERT again closes it and gives focus
  back. The overlay hides when neither the game nor the overlay has focus, or the game is minimised. Home shows the
  logo + name, PID, module bases, match status, active features (local pawn re-read at ~4 Hz), overlay size and FPS. Settings has live Watermark and
  Frame outline switches. The exe carries the logo as its icon. Exit: Ctrl+C / closing the console, Alt+F4 with the menu open, or CS2 closing.

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
- **RPM/WPM report a bad remote address through the return value, not an exception.** The `__try/__except` in
  `core::ProcessMemory::do_read` only protects our side of the copy. Functions containing `__try` must not hold C++
  objects with destructors (C2712), which is why the guard lives in the non-template `do_read`/`do_write`.
- **A partial read is a failed read.** `ProcessMemory` requires `bytes_read == size`; `safe_read` reads into a
  temporary, so on failure the caller's variable is untouched.
- **Implausible requests never reach RPM.** `core::Memory::read_bytes` rejects null buffers, empty sizes and ranges
  outside user space before calling the implementation (tested with `FakeMemory::read_count()`).
- **`CreateToolhelp32Snapshot(TH32CS_SNAPMODULE)` can fail with `ERROR_BAD_LENGTH`** while the game is still loading
  modules. `core::module_base` retries.

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
- **Alt-tab behaviour:** the overlay should stay topmost over the game but not over other windows. Done by polling
  `GetForegroundWindow()` every frame (more robust than `WM_ACTIVATE`, which the click-through overlay never gets):
  visible only while the game or the overlay is in front.
- **Per-pixel alpha needs the blt model.** `DwmExtendFrameIntoClientArea(-1)` + a `DXGI_SWAP_EFFECT_DISCARD` swap chain
  cleared to (0,0,0,0). A flip-model swap chain on an HWND ignores alpha (that would need DirectComposition).
- **`WS_EX_TRANSPARENT` only passes clicks through on a `WS_EX_LAYERED` window.** The layered window uses
  `LWA_ALPHA 255`, so with click-through off it takes every click over the game, not only those on the menu.
- **Opening the menu must take focus**, otherwise the game keeps the mouse (hidden, relative) and still shoots/turns.
  `SetForegroundWindow` from a background process is normally refused. It works here because the menu key is a
  `RegisterHotKey`: receiving `WM_HOTKEY` counts as user input to our process. If it ever fails, the Home page and
  the console say to click the menu once. Phase 7's `GetAsyncKeyState` polling must keep a hotkey (or another input
  path) for this, or the focus grab breaks.
- **The hotkey swallows the key system-wide while registered**, so it is registered only while CS2 or the overlay
  has focus.
- **`LONG_PTR` style masks:** `~` on a `DWORD` style mask zero-extends when widened and clears the high bits.
  Keep masks as `LONG_PTR` (`kClickThroughStyles`).
- **CS2 must be in Windowed or Fullscreen Windowed** for the overlay to show on top. Exclusive fullscreen can minimise
  the game or hide topmost windows.
- **While the menu is open, the game runs unfocused**, so CS2's `engine_no_focus_sleep` drops the game's FPS. Normal.
- **ImGui 1.92.8+: `ImDrawList::AddRect(min, max, col, rounding, thickness, flags)`.** The old `flags, thickness`
  order is `= delete`d under `IMGUI_DISABLE_OBSOLETE_FUNCTIONS` (C2280). Same for `AddPolyline` and `PathStroke`.

### Tests (doctest)
- **Define `DOCTEST_CONFIG_USE_STD_HEADERS`** (set in `tests.vcxproj`).
- doctest is on the **external** include path with external warnings off.

---

## 11. Dos and don'ts

**Do**
- Respect §1. If a task drifts toward online use, evasion, stealth, networking or distribution, stop and tell the
  user.
- Open the process handle with the minimum access rights needed.
- Read and write game memory only through `core::Memory` (`safe_read<T>` / `read<T>` / `safe_write<T>`). Code that
  only reads takes `const core::Memory&`; code that writes takes `core::Memory&`. Never a raw `RPM`/`WPM` call
  outside `core/process_memory`. Test `game/` code with `tests/helpers/fake_memory.h`.
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
  `src/external` (plus `core/process_memory.cpp` and `core/pattern.cpp`, which are Win32 but harmless to test) and
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
- [x] Solution, `external` (console exe) and `tests` projects, shared `props/common.props` (C++20, x64, `/MT`,
      `/W4 /WX`)
- [x] `.gitignore`, `.gitattributes`, README stub, `docs/DEVLOG.md`, `docs/offsets.md`, `docs/dumps/`,
      `external/README.md`, this file
- [x] `core/process`: find `cs2.exe` (Toolhelp32), open a handle, look up `client.dll` and `engine2.dll` module
      bases
- [x] `core/memory`: `safe_read<T>` / `safe_write<T>` wrappers around `ReadProcessMemory` / `WriteProcessMemory`,
      with `__try/__except` guards and an `is_plausible_pointer` helper (as the `core::Memory` interface +
      `core::ProcessMemory`, with `FakeMemory` for tests)
- [x] `core/log`: `logger::info/warn/error`
- [x] `main.cpp`: open the handle, print the module bases and the local pawn pointer, wait for Enter, exit
- [x] Builds with zero warnings (Debug + Release); `tests.exe` 13/13 cases pass in both
- [x] Verified in-game by the user (launch with `-insecure`, offline bots): 2026-10-06, build 14189, offline
      deathmatch: PID, both module bases and local pawn `0x4DF564BF800` printed; game closed → "cs2.exe not found".
      Not exercised: main menu (`none`) and the access-denied path (the game isn't elevated on this PC).

**Acceptance:** the tool finds `cs2.exe`, prints the PID, the `client.dll` base, the `engine2.dll` base, and the
local pawn pointer (plausible hex). It exits cleanly. Running it with the game closed says "cs2.exe not found."
Running it without admin, if the game is elevated, says "OpenProcess failed" and tells the user to run as admin.

### Phase 1: Overlay window + ImGui shell
- [x] Vendor Dear ImGui (pinned tag) + Win32 + DX11 backends (v1.92.9b, `external/vendor.vcxproj`)
- [x] `ui/overlay_window`: transparent, click-through, topmost Win32 window with its own DX11 swap chain; tracks
      the game window's client rect; toggles `WS_EX_TRANSPARENT` on INSERT
- [x] `ui/imgui_layer`: ImGui context + backends for the overlay window; shutdown on exit
- [x] `ui/theme` (dark), `ui/menu` with sidebar pages (ESP / Aimbot / Triggerbot / Misc / Settings,
      placeholders), branding "External Cheat - by BigH"
- [x] Logo (moved up from Phase 10 at the user's request): exe icon, menu header, Home, watermark, README
- [x] Watermark lists the active features one per line (`features/feature_summary`, tested); Player page removed;
      no menu-key hints in the watermark or the header; header keeps only the "External" pill
- [x] INSERT toggles the menu (temporary hard-coded until Phase 7)
- [x] Builds with zero warnings (Debug + Release); `tests.exe` 17/17 in both
- [x] Verified in-game by the user (2026-10-06): menu open over an offline bot match (screenshot: 1920x1080,
      270 FPS, "in a match"), logo/watermark/header iterated on twice, then approved for commit

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
state, origin, view offset, eye angles, weapon services, active weapon, shots fired, dormant, flags, scoped,
spotted state, crosshair entity index, flash duration) and the `jump` / `attack` button globals, and every signature
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
- [ ] `settings/settings.h` (ESP section; the overlay options move in) + `app/frame` fills
      `AppState::active` from it, so the watermark shows "ESP"
- [ ] `features/esp`: pure decision; `render/primitives` + `render/painter` on the overlay's ImGui background
      draw list
- [ ] ESP page: 2D box, corner box, head circle, skeleton (optional), name, health bar/number, distance, weapon
      name, snaplines (origin choice), team mode, enemies only, colours with opacity, thickness
- [ ] Scoped indicator: a "SCOPED" tag on bots with `m_bIsScoped` set (option on the ESP page)
- [ ] **Before building any visibility check, stop and remind the user of the three options and let them choose**
      (they asked for this on 2026-10-06): (1) the game's own trace = internal, out of scope; (2) the spotted-by
      mask = external, no map geometry, cheap, but slightly delayed and "spotted" rather than per-pixel visible;
      (3) our own ray cast against the map's collision mesh parsed from the game files = external, exact and
      per-frame, but a big project (Valve file formats, a BVH, no moving doors/props). Default plan is (2).
- [ ] Visible / hidden colours from the spotted-by heuristic (`game/visibility`: `is_spotted_by(pawn, local slot)`,
      tested against FakeMemory)
- [ ] Builds with zero warnings (Debug + Release); tests pass
- [ ] Verified in-game by the user

**Acceptance:** boxes line up with bots at near/far distance and different resolutions; nothing drawn for bots
behind you; skeleton faces the right way (if bones are done); colours/opacity change live; dead bots not drawn;
map change doesn't crash; a bot scoped in with an AWP shows "SCOPED" and loses it when it unscopes; visible /
hidden colours follow line of sight (with the heuristic's small lag); tests pass (including the row-major matrix
centre test).

### Phase 5: Aimbot + triggerbot
- [ ] `maths/angles`: `Angles`, `normalize`, `clamp`, `calc_aim_angles`, `angular_distance`, `is_within_fov`,
      `smoothing_fraction`, `step_towards`; tests
- [ ] `features/aimbot`: candidates (FOV, dead, team, max distance), priority (crosshair / distance / lowest
      health), head/body/nearest-bone, compute aim; tests
- [ ] `features/triggerbot`: state machine (idle → target acquired → reaction delay → firing → between shots),
      fire modes single / burst / hold, filters (team, visible, distance, weapon class, scoped, flashed, in air);
      tests for every option
- [ ] Write handle (`PROCESS_VM_WRITE | PROCESS_VM_OPERATION`); angle write via `WriteProcessMemory` on the main
      thread; triggerbot fires via the `attack` button (or `SendInput`, decided with an in-game test); nothing fires
      or aims while the menu is open or the game isn't focused
- [ ] Visible-only option for the aimbot (spotted-by heuristic)
- [ ] Aimbot page + Triggerbot page: enable, activation key (hold/toggle), aim point, priority, FOV radius +
      circle, smoothing, team check, max distance, visible only; triggerbot: every option in §3
- [ ] Builds with zero warnings (Debug + Release); tests pass
- [ ] Verified in-game by the user

**Acceptance:** holding the aim key locks on the right target for each priority; head/body/nearest-bone works;
smoothing feels consistent at different framerates; FOV circle matches which bots are eligible; team check and
max distance filter correctly; visible-only skips bots behind walls; triggerbot fires only with an enemy under the
crosshair, and each option (activation, delay, fire mode, filters) behaves as labelled; neither feature fires while
the menu is open or the game isn't focused; tests pass.

**Honest note:** external aimbot cannot write angles on the game's logic thread, so the view will feel slightly
laggier than an internal one. That's the cost of external. It's still usable for offline learning.

### Phase 6: Misc: bunny hop, radar, bomb timer, spectators, hitsound
(Replaces the dropped "Player values" phase: no Player page, no health / armour / ammo writes, no Set / Freeze.)
- [ ] `features/bunny_hop` (pure, tested): space held + on ground → press jump; in air → release. Diagnostic first
      (in-game): the right value format for the `jump` button write, and whether the loop rate catches every landing
      (if not: a faster polling path for this one check, decided with the user)
- [ ] Misc page: bunny hop on/off + its key (space by default), radar, bomb timer, spectator list, hitsound
- [ ] Radar, bomb timer (`dwPlantedC4`), spectator list (observer handles), hitsound (overlay audio): one at a time,
      each with its own in-game check
- [ ] Builds with zero warnings (Debug + Release); tests pass
- [ ] Verified in-game by the user

**Acceptance:** holding space chains hops on flat ground without a manual re-press; releasing space stops it
immediately; nothing jumps while the menu is open or the game isn't focused; each misc feature does what its label
says; tests pass.

### Phase 7: Keybind engine
- [ ] `input/keys`, `input/actions` (registry + defaults), `input/keybinds` (HOLD/TOGGLE/PRESS), tested
- [ ] `input/bind_capture` (pure, tested)
- [ ] `input/key_poll` on the main thread; act only while the game is focused; suspended during capture
- [ ] Keybinds page: every action by category, click to capture, mode selector, conflict highlight; bind buttons
      on each page
- [ ] Actions: menu toggle (INSERT), panic, exit, aimbot (HOLD/TOGGLE), aimbot enable, ESP toggle, triggerbot
      (HOLD/TOGGLE), presets
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
keybinds and colours alone; tests pass.

### Phase 9: Panic, clean shutdown, polish
- [ ] Panic: disables aimbot/ESP/triggerbot (so the angle writes stop), closes the menu
- [ ] Exit action + Settings-page button: restore everything, shut down the overlay, close the handle, exit
- [ ] Final key layout: **DELETE = exit, END = panic** (INSERT menu)
- [ ] Robustness review: map change, death, alt-tab, minimise, game close, game restart
- [ ] Builds with zero warnings (Debug + Release); tests pass
- [ ] Verified in-game by the user

**Acceptance:** panic returns the game to normal instantly; 10 start/stop cycles with no crash, leak, input or
cursor problem; after exit the game behaves exactly like an untouched game.

### Phase 10: UI redesign
- [x] Rename to the final name (decided with the user): **External Cheat - by BigH**, done in Phase 1
- [x] Logo embedded (menu header + Home via an ImGui user texture): done early, in Phase 1
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

- **Phase 0: done, verified in-game (2026-10-06, CS2 build 14189), approved, committed and pushed** to
  `github.com/BigH018/cs2-external` (private, `main`) as three commits: build setup, core code + tests, docs + dumps.
- **Dumps:** `docs/dumps/` holds `info.json`, `offsets.json`, `client_dll.json`, `interfaces.json` and
  `buttons.json`, copied byte-for-byte from the user's a2x/cs2-dumper run (build 14189, 2026-10-06T16:04:45Z). The
  dumper's git commit isn't in `info.json` and isn't recorded; build number + timestamp are the provenance.

- **Phase 1: done, verified in-game and approved by the user (2026-10-06), committed and pushed.** Debug + Release
  build with zero warnings, tests 17/17. After the user's first look: logo added everywhere (exe icon, menu header,
  Home, watermark, README), the watermark lists the active features, the Player page/phase was dropped. A smoke test with CS2 running but not focused found the device, ImGui and
  the game window, and the overlay correctly stayed hidden. INSERT, focus, click-through and alignment are untested
  until the user tries them in-game.

- **Feature plan updated (2026-10-06):** bunny hop (Phase 6, which replaced the dropped Player phase), ESP scoped
  indicator + visible colours (Phase 4), visible-only aimbot and a fully configurable triggerbot (Phase 5). All
  external; visibility is the spotted-by heuristic (§3 feasibility table).

**Next:** Phase 2 (offsets, signatures, interfaces, schema), in a new session. Include the new fields in §7's
"Fields and buttons the planned features need" table.

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
- **2026-10-06 (Phase 0):** Repo created: `github.com/BigH018/cs2-external`, **private**, default branch `main`, the
  user (BigH018) is the only collaborator. Commits use the user's git identity with no AI attribution lines.
- **2026-10-06 (Phase 0):** The Phase 0 handle is **read-only** (`PROCESS_VM_READ |
  PROCESS_QUERY_LIMITED_INFORMATION`, `core::kReadOnlyAccess`). Write access is added only in the phase whose
  feature needs it (Phase 5 angle writes; Phase 6 player values was dropped later the same day).
- **2026-10-06 (Phase 0):** Offset and schema constants keep the **dumper's exact names** (`dwLocalPlayerPawn`,
  later `m_iHealth`) instead of `kPascalCase`, so they can be grepped across dumps. A deliberate exception to §9.
- **2026-10-06 (Phase 0):** The local-pawn read lives in `game/player.cpp` (`read_local_pawn`) rather than in
  `main.cpp`, to keep the "only `game/` dereferences game memory" rule from day one. `game/offsets.h` holds only
  `dwLocalPlayerPawn` until Phase 2 copies in the rest.
- **2026-10-06 (Phase 0):** doctest v2.5.3 vendored now (already in the §4 stack) so the `tests` project exists from
  Phase 0, with `core/memory` tests against our own process.
- **2026-10-06 (Phase 0):** `common.props` is imported after `Microsoft.Cpp.props`; `WindowsTargetPlatformVersion`
  and `CharacterSet` live in each `.vcxproj` (see §4). Same layout as the AC project.
- **2026-10-06 (Phase 0, user said re-architecting is fine):** reviewed the AC project (`BigH018/internal-assault-cube`)
  and adopted its patterns:
  - **`core::Memory` is an interface** (pure header) with `ProcessMemory` (RPM/WPM) as the real implementation and
    `tests/helpers/fake_memory.h` for tests. This is the external version of AC's pure `safe_memory.h`: AC could
    test `game/` code because game memory was local; externally every read is RPM, so `game/` code takes a
    `core::Memory&` and the Phase 3 entity-list tests run against a fake chunked entity list. Read-only code takes
    `const Memory&`, so "this code can't write" is checked by the compiler. Range checks happen once in the base
    (NVI: public `read_bytes` → private virtual `do_read`). A virtual call per RPM is negligible next to the syscall.
  - **`config.h` exists from Phase 0** and holds the pointer bounds (`kMinValidPointer`/`kMaxValidPointer`) and the
    process/module names, like AC's `config.h`.
  - **Logger = AC's:** `noexcept`, drops the line if formatting throws, `[+]` / `[!]` / `[x]` prefixes.
  - **`app/` (frame, state, live_view)** is added to the plan as the orchestrator from Phase 1; `main.cpp` stays a
    bootstrap.
  - **Code style = AC's:** namespace contents not indented, closing `} // namespace x`.
- **2026-10-06 (Phase 0):** CLAUDE.md's earlier "3301 classes / 569 enums" for `client_dll.json` was wrong. The real
  build-14189 dump has 542 classes and 14 enums (the copy the user pasted was complete).
- **2026-10-06 (Phase 1):** Dear ImGui pinned to **v1.92.9b** (commit `f1cc2ae`), the same tag as the AC project, with
  the **DX11** backend instead of AC's OpenGL2. Compiled in `vendor.lib` (warnings off); config defines in
  `props/imgui.props`, as in AC.
- **2026-10-06 (Phase 1):** Overlay transparency = layered popup + `DwmExtendFrameIntoClientArea(-1)` + blt-model
  swap chain, not DirectComposition (less code, no `dcomp` dependency; works on Windows 10/11).
- **2026-10-06 (Phase 1):** Opening the menu **takes focus** (`SetForegroundWindow` on the overlay, allowed because
  INSERT is a `RegisterHotKey`) and closing hands it back. This is how "clicks in the menu don't shoot" works
  externally: the game stops getting input because it isn't focused. The planned `input/cursor` (CursorControl) was
  **dropped**: our window owns its own cursor via ImGui's `WM_SETCURSOR` handling, and the game releases its own when
  unfocused. Revisit only if the in-game test shows a cursor problem.
- **2026-10-06 (Phase 1):** Visibility follows `GetForegroundWindow()` polled every frame (game or overlay in front →
  shown), not `WM_ACTIVATE`.
- **2026-10-06 (Phase 1):** Added `ui/hud` (watermark + optional frame outline, drawn every frame) so the overlay is
  visibly running and its alignment can be checked in-game. Overlay options live in `app::OverlayOptions` until
  the `Settings` struct (now Phase 4).
- **2026-10-06 (Phase 1):** Exit paths for now: Ctrl+C / closing the console (console control handler →
  `core::shutdown_requested`; for a console close, the handler waits up to 3 s for `shutdown_complete`), Alt+F4 on
  the focused menu, or CS2 exiting. DELETE/END come in Phase 9.
- **2026-10-06 (Phase 1, user request):** **Player values dropped**: no Player page, no health / armour / ammo writes,
  no Set / Freeze (Phase 6 kept as "DROPPED" so phase numbers don't shift). The only planned game write left is the
  aimbot's view angles (Phase 5). `settings/settings.h` moves to Phase 4, where the first setting (ESP) appears.
- **2026-10-06 (Phase 1, user request):** **Logo moved up from Phase 10**, done the AC way: `assets/logo.jpg` (README +
  source of `ui/logo_pixels.h`, generated by `tools/make_logo_header.py`, committed so builds need no Python),
  `assets/logo.ico` (exe icon via `external.rc`). In the overlay it's an ImGui user texture (no image decoder
  dependency). The user's files `miraikitsu-chibi-gojo.{jpg,ico}` were renamed to `assets/logo.{jpg,ico}`.
- **2026-10-06 (Phase 1, user request):** The **watermark shows the logo and the active features** ("ESP · Aimbot";
  "No features on" when none). The list comes from the pure `features/feature_summary` (`ActiveFeatures` in
  `AppState::active`, filled from the settings by `app/frame` once features exist). Sidebar groups are now COMBAT
  (Aimbot, Triggerbot), VISUALS (ESP, Misc), SETUP (Settings).
- **2026-10-06 (Phase 1, user request):** Renamed to **External Cheat - by BigH** (`config::kAppName`, overlay window
  class/title). The watermark lists the active features **vertically** (one per line, accent dot) under the title and
  no longer shows the menu key. The menu header lost its "In match" and "INSERT closes" pills (only "External"
  stays; match status is still on Home). Home's header is just the logo and the name (subtitle removed).
  `features::active_feature_names()` added for the vertical list; `feature_summary()` (joined) is kept for Home.
- **2026-10-06 (user request):** New features, all **external** (feasibility table in §3): **bunny hop** (Misc,
  Phase 6, via `m_fFlags` + a `jump` button write), **ESP scoped indicator** (`m_bIsScoped`, Phase 4), **visibility**
  for ESP/aimbot/triggerbot as the **spotted-by heuristic** (`m_entitySpottedState.m_bSpottedByMask`), because exact
  line of sight needs the game's trace (internal). **Triggerbot made fully configurable** (activation, delay, fire
  modes, filters; Phase 5). Phase 6 became "Misc" (bunny hop + radar, bomb timer, spectators, hitsound, previously
  unscheduled). Button writes (jump, attack) join the aimbot's angle write as the only game writes; the write handle
  arrives in Phase 5. The UI placeholders and `features::ActiveFeatures` (now with `bunny_hop`) match this plan.

---

## 16. Maintenance rule

**After EVERY task**, before finishing:
1. Update the **file tree** (§5): status markers, new/removed files, one-line descriptions.
2. Tick the **roadmap** checkboxes (§13) and update **Current status** (§14) and its **Next** line.
3. Update `docs/offsets.md` with every offset, signature, and schema field found this session, plus its source
   (repo + commit + date) and how it was proven.
4. Update any rules, gotchas or patterns that changed (§4–§12) and add to the **Decision log** (§15).
5. Add a dated entry to `docs/DEVLOG.md` (what was built, bugs found and how they were fixed).
