# Dev log

## 2026-10-06: Phase 0, scaffold and process handle

**Built**
- Private GitHub repo `BigH018/cs2-external`; initial commit holds `CLAUDE.md`, `.gitignore`, `.gitattributes`.
- `cs2-external.sln` (Debug|x64, Release|x64) with two projects: `external` (`cs2_external.exe`) and `tests`
  (`tests.exe`). Shared settings in `props/common.props`.
- `core/process`: `find_process` (Toolhelp32 process snapshot), `open_handle` (OpenProcess with an exact access
  mask), `module_base` (Toolhelp32 module snapshot, retries on `ERROR_BAD_LENGTH`), and the `UniqueHandle` RAII
  wrapper.
- `core/memory`: `read_bytes` / `write_bytes` (`__try/__except`-guarded RPM/WPM, exact byte count required),
  `safe_read<T>`, `read<T>` (optional), `safe_write<T>`, and `is_plausible_pointer` (constexpr).
- `core/log`: `logger::info/warn/error` using `std::format`.
- `game/offsets.h` with `dwLocalPlayerPawn` only; `game/player` with `read_local_pawn`.
- `main.cpp`: finds `cs2.exe`, opens a **read-only** handle, prints the PID, `client.dll` and `engine2.dll` bases, and
  the local pawn pointer, then waits for Enter.
- Vendored doctest v2.5.3; `tests/core/test_memory.cpp` (9 cases, 39 assertions) runs the RPM/WPM wrappers against
  our own process, including a reserved-but-uncommitted page and a read that runs off the end of a committed page.

**Problems and fixes**
- Property sheets imported before `Microsoft.Cpp.props` get overridden by the toolset defaults. `common.props` is
  now imported after it. `WindowsTargetPlatformVersion` and `CharacterSet` have to be set *before* it, so those two
  live in each `.vcxproj`.

**Verified here:** Debug and Release build with zero warnings; tests pass in both; with CS2 closed the tool prints
`cs2.exe not found`.

### Later the same day: in-game check + AC-inspired refactor

- **In-game (build 14189, offline deathmatch, frozen bots):** PID 18592, `client.dll` `0x7FFD62930000`,
  `engine2.dll` `0x7FFDD9CE0000`, local pawn `0x4DF564BF800`. Phase 0 acceptance met. Main menu (`none`) and the
  access-denied path weren't exercised.
- **Dumps:** copied `info.json`, `client_dll.json`, `interfaces.json`, `buttons.json` (and the identical
  `offsets.json`) from the user's dumper output. `info.json` gives the build (14189). The real `client_dll.json` has
  542 classes / 14 enums, not the 3301 / 569 CLAUDE.md used to claim.
- **Refactor after reading the AC project:** `core::Memory` became a pure interface (`read_bytes` → private virtual
  `do_read`) with `core::ProcessMemory` (RPM/WPM, the only Win32 part) and `tests/helpers/fake_memory.h`. `game/`
  code now takes `const core::Memory&`, so it's testable without the game (`tests/game/test_player.cpp`). Added
  `config.h` (pointer bounds, process/module names). Logger switched to AC's noexcept `[+] [!] [x]` version.
  Reformatted to AC's style (unindented namespaces).
- **Tests:** 13 cases / 57 assertions, Debug and Release. Re-ran the tool in-game after the refactor: same output.

## 2026-10-06: Phase 1, overlay window + ImGui shell

**Built**
- Vendored Dear ImGui **v1.92.9b** (core + Win32 + DX11 backends; 15 files, every blob hash checked against the tag)
  and compiled it in a new static lib, `external/vendor.vcxproj`. ImGui config defines live in `props/imgui.props`.
- `ui/overlay_window`: our own `WS_POPUP` window (`TOPMOST | LAYERED | TOOLWINDOW`, plus `TRANSPARENT | NOACTIVATE`
  while the menu is closed) with its own D3D11 device and swap chain. Per-pixel transparency comes from
  `DwmExtendFrameIntoClientArea(-1)` plus a blt-model (`DXGI_SWAP_EFFECT_DISCARD`) swap chain cleared to (0,0,0,0).
  It covers the game's client rect, resizing the swap chain when the game window changes size.
- INSERT is a `RegisterHotKey` on the overlay, registered only while CS2 or the overlay has focus. Opening the menu
  drops `WS_EX_TRANSPARENT | WS_EX_NOACTIVATE` and calls `SetForegroundWindow(overlay)`: the game loses focus, lets go
  of the mouse, and stops getting clicks. Closing restores click-through and hands focus back to the game. Focus going
  anywhere else (Alt+Tab, the game's title bar) closes the menu; the overlay hides whenever neither CS2 nor the overlay
  has focus, or the game is minimised.
- `ui/imgui_layer` (context, Segoe UI fonts, backends), `ui/theme` (the AC navy palette), `ui/widgets` (page header,
  cards, info rows, pills, "planned" cards), `ui/menu` (header + grouped sidebar), `ui/pages/*` (Home with live status;
  ESP/Aimbot/Triggerbot/Player/Misc placeholders; Settings with live watermark / frame-outline switches), `ui/hud`
  (watermark + frame outline on the background draw list).
- `app/frame` + `app/state`: the orchestrator loop. The Home page's match status re-reads the local pawn at ~4 Hz.
- `core/process`: `find_main_window(pid)` (largest visible unowned top-level window) and `is_running(handle)`;
  `core/runtime.h`: `shutdown_requested` / `shutdown_complete`, set by the console control handler (Ctrl+C, console
  closed). `main.cpp` makes the process per-monitor DPI aware v2 before any window exists.

**Problems and fixes**
- ImGui 1.92.8+ swapped `ImDrawList::AddRect`'s last two parameters (`thickness` before `flags`). The old order is
  `= delete`d under `IMGUI_DISABLE_OBSOLETE_FUNCTIONS`, so it was a compile error (C2280), not a silent bug.
- `handle_message` first used `hwnd_`, which is still null during the messages `CreateWindowExW` sends
  (`WM_NCCREATE`...). It now uses the `hwnd` it receives.

**Verified here:** Debug and Release build with zero warnings; `tests.exe` 13/13 in both. Smoke test with CS2 running
(not focused): device (feature level 11.0), ImGui and the game window were all found, and the overlay stayed hidden.
In-game check by the user still to do.

### Later the same day: logo, active-feature watermark, Player dropped (user feedback)

- **Logo** (moved up from Phase 10), the AC way: the user's `miraikitsu-chibi-gojo.{jpg,ico}` became
  `assets/logo.{jpg,ico}`. `tools/make_logo_header.py` (Pillow) turns the jpg into `src/external/ui/logo_pixels.h`
  (128x128 RGBA, committed); `ImGuiLayer` registers it as an ImGui user texture, which the DX11 backend uploads and
  releases itself. It shows in the menu header, on Home (96 px) and in the watermark. `external.rc` makes the ico the
  exe's icon. README shows the jpg.
- **Watermark:** `[logo] Internal Cheat by BigH | <active features> | INSERT: menu`. The feature list comes from the
  new pure `features/feature_summary` ("ESP · Aimbot", fixed order; "No features on" when empty), 4 new test cases.
  Nothing can be switched on yet, so it says "No features on" until Phase 4 fills `AppState::active` from the settings.
  Home's status card shows the same line.
- **Player page and Phase 6 dropped** at the user's request (no health/armour/ammo writes). Sidebar: COMBAT (Aimbot,
  Triggerbot), VISUALS (ESP, Misc), SETUP (Settings). Roadmap and docs updated; Phase 6 stays as "DROPPED".
- `.gitattributes`: `*.jpg` / `*.jpeg` marked binary.

**Verified here:** Debug and Release build with zero warnings, tests 17/17 in both. In-game check of the new watermark,
logo and menu still to do.

### Later the same day: name, watermark layout, header (user feedback with a screenshot)

- Renamed to **External Cheat - by BigH** (`config::kAppName`; overlay class `ExternalCheatOverlay`).
- Watermark: logo + "External Cheat by BigH", then the active features **one per line** with an accent dot ("No
  features on" when none). The "INSERT: menu" part is gone. New `features::active_feature_names()` returns the list;
  tests updated (17 cases / 66 assertions).
- Menu header: the "In match" and "INSERT closes" pills are gone; only the "External" pill stays.
- Home: the subtitle under the name is gone; the name sits beside the logo, centred on it.

**Verified here:** Debug and Release build with zero warnings, tests 17/17 in both.

### Later the same day: feature plan update, Phase 1 approved

- The user asked for a bunny hop, an ESP scoped indicator, a visibility check for aimbot/ESP and a more configurable
  triggerbot. All external; checked against the build-14189 dump: `m_fFlags` + the `jump` button (bunny hop),
  `m_bIsScoped` (scoped), `m_entitySpottedState.m_bSpottedByMask` (visibility heuristic: exact line of sight would
  need the game's trace, i.e. internal), `m_iIDEntIndex` + the `attack` button + `m_flFlashDuration` (triggerbot).
  CLAUDE.md §3 has a feasibility table, §7 the field list, the roadmap the new items (Phase 6 is now "Misc", replacing
  the dropped Player phase). `docs/offsets.md` lists the values as planned (not in code yet).
- Caught while writing the docs: two button offsets typed from memory as hex were wrong; converted from the dump's
  decimal values instead (`jump` `0x22324E0`, `attack` `0x2231FD0`). Lesson already in CLAUDE.md: copy, never retype.
- UI placeholders (ESP, Aimbot, Triggerbot, Misc) list the new options; `features::ActiveFeatures` gained
  `bunny_hop` (tests updated).
- Phase 1 approved by the user after the in-game check (menu over an offline bot match, 1920x1080, 270 FPS).

## 2026-10-06: Phase 2, offsets, signatures, interfaces, schema

CS2 was running (build 14189, `-insecure`, an offline bot match), so every layout was proven against the live process
with a read-only Python/ctypes script (scratch, not in the repo) **before** the C++ was written, instead of guessed.

**Built**
- `game/offsets.h`: all 29 dumped globals (client, engine2, inputsystem, matchmaking, soundsystem), all 16 buttons,
  the RVAs of the 4 interfaces we use, `kDumpBuildNumber = 14189`, 8 signatures, and the hand-found engine layouts
  (`offsets::layout`). Values generated by a script from `docs/dumps/*.json`, not retyped.
- `game/schema.h`: 29 fields across 13 classes (everything the Phase 2 acceptance list and Phases 3-6 need), plus
  `schema::kFields`, the table the diagnostic walks.
- `core/pattern`: IDA-style `Pattern` (parse, `matches_at`, `find_all` with a memchr anchor), `copy_remote` (1 MiB
  reads, page-by-page fallback, unreadable pages zero-filled and counted), `rip_relative` / `rip_relative_target`.
- `core/pe`: a loaded module's headers, sections and exports read through `core::Memory` (forwarders rejected).
- `core::read_string` (in `memory.h`): page-bounded string reads.
- `game/interfaces`: `CreateInterface` from outside. Its first instruction (`mov r9, [rip+x]`) gives the head of the
  module's `InterfaceReg` list; each create function (`lea rax, [rip+instance]; ret`) is decoded, not called.
- `game/schema_system`: the type scope from `SchemaSystem_001` (vector at +0x190), and every class info found in one
  pass over a copy of client.dll (class infos are static data whose first qword points to itself).
- `game/signatures`: resolve a signature over the copy; several matches are fine if they all agree.
- `app/diagnostics` + `main.cpp`: the startup diagnostic (build, interfaces, signatures, schema, buttons/globals).
  `cs2_external.exe --diag` prints it and exits (exit code 0 = all OK, 2 = something failed). The Home page shows
  "Offsets: build 14189: all 45 checks OK".
- Tests: `test_pattern`, `test_pe`, `test_interfaces`, `test_schema_system`, `test_signatures`, `test_offsets`, and
  `read_string` cases; `helpers/fake_pe.h` builds a tiny PE32+ in a FakeMemory; FakeMemory got `put_bytes` /
  `put_string`. 41 cases / 652 assertions.

**Found along the way**
- The `InterfaceReg` walk matched `interfaces.json` for all 59 interfaces in client, engine2, schemasystem,
  inputsystem and tier0.
- The live schema system matched the dump for **3013 fields in 469 classes**, 0 mismatches. The dump's 542 also has
  classes from other libraries (entity2, pulse_runtime_lib, compositematerialslib) registered in the client scope.
- `dwLocalPlayerPawn` has no code reference of its own: it's a field of the prediction object (`dwPrediction + 0xF8`).
  Its signature finds the prediction getter and adds 0xF8.
- Of the a2x-style signatures tried first, five still hit; `dwEntityList`, `dwPlantedC4` and the prediction one
  didn't, or hit the wrong place, and were rebuilt from the stores to each global.
- `GameEntitySystem` isn't a registered interface in CS2; it's the global `dwGameEntitySystem` (= `dwEntityList`).
- **CLAUDE.md §7 had three wrong facts**, all proven live and corrected: the weapon item-definition chain is `0x149A`
  (not `0x14FA`: an AK-47 reads 7 there, `0x14FA` reads 0); an entity identity is `0x70` bytes (not `0x78`); and
  `identity + 0x10` holds the whole handle (`0x1B182E6`), not just the serial.
- `dwNetworkGameClient_isBackgroundMap` in the dump (`0x2C143F`) doesn't look like a field offset; copied but flagged.

**Problems and fixes**
- A unit test mapped a 0x40-byte FakeMemory region for strings; `read_string` reads up to the page end, which real
  memory always allows (mapping is per page), but FakeMemory doesn't. The test now maps a whole page.

**Verified here:** Debug and Release build with zero warnings; `tests.exe` 41/41 in both. `cs2_external.exe --diag`
(Debug and Release) against the running game: **all 45 checks OK**; copying client.dll (41 MiB) takes ~8 ms.
In-game check by the user: normal start in an offline bot match printed all 45 checks OK and the overlay started.
Approved and committed.

## 2026-10-06 (night, autonomous mode): Phase 3, entity list + snapshots

Auto-approved: user was asleep and did not personally verify this phase. If in-game testing fails, this commit is the
first suspect for rollback.

CS2 was running (build 14189, de_mirage, a bot match with frozen bots), so the layouts were proven with a read-only
Python script first, then the C++ was checked with the new `--live` view.

**Built**
- `game/handle`: entity system pointer, identity address (chunk + slot × 0x70), `entity_at`, `resolve_handle` (the
  identity's handle must equal the handle, so a reused slot doesn't resolve).
- `game/entities`: `designer_name`, `find_player_controllers` (indices 1..maxClients named `cs_player_controller`).
- `game/player`: `read_player` (controller: name, team, pawn handle, alive; pawn: health, life state, position, eye
  height, armour, flags, scoped, dormant, weapon) with validity checks; `read_game` (globals, view matrix if sane,
  every player, which one is local).
- `game/globals` (CGlobalVars in one read), `game/view` (view matrix), `game/weapon` (pawn → weapon id → name and
  class), `game/snapshot.h`, `maths/vec.h`, `maths/projection.h` (`ViewMatrix` + `is_sane`).
- `app/live_view` + `--live`: a console table at ~4 Hz, in place (VT escapes) or as plain frames when redirected.
- Home page: Map, Players, You rows from a ~4 Hz snapshot.
- Tests: `helpers/fake_entities.h` (a fake chunked entity system) and 29 new cases (handle, entities, player, globals,
  view, weapon, vec). 70 cases / 1264 assertions.

**Found along the way**
- `CGlobalVars` doesn't have the "typical" layout CLAUDE.md gave: in build 14189, `+0x10` is maxClients (64), curtime
  is at `+0x30`, tickcount at `+0x44`, the map name at `+0x188`. Proven by sampling twice 2 s apart. CLAUDE.md fixed.
- A pawn's designer name is `c_cs_player_for_precache`, not `cs_player_pawn`.
- A full snapshot of 20 players costs ~0.33 ms (≈600 RPM calls): fine to do every frame for the ESP.

**Problems and fixes**
- Scripted edits through a Bash heredoc lost a level of backslashes: `game\view.cpp` turned into a vertical tab
  (MSBuild: "hexadecimal value 0x0B is an invalid character"), `app\frame` / `game\test_*` replacements silently
  didn't match (LNK2019 for `run_live_view`, missing tests), and `'\n'` became a raw newline in C++. Fixed with the
  Edit tool; backslash text is no longer edited through heredocs.
- `--live` redirected to a file failed ("no virtual terminal support"); it now falls back to plain frames.

**Verified here:** Debug and Release build with zero warnings; `tests.exe` 70/70 in both. `--live` (Release) against
the running game: 20 players, names/teams/weapons/positions right, globals ticking at 64 Hz, view matrix sane.
`--diag`: all 45 checks OK. The normal start still brings up the overlay and logs "In a match".
**Not exercised in-game:** death and respawn, a match restart, a map change, the main menu (covered by unit tests
only). Bots were frozen, so moving positions weren't seen either.

## 2026-10-06 (night, autonomous mode): Phase 4, world-to-screen + ESP

Auto-approved: user was asleep and did not personally verify this phase. If in-game testing fails, this commit is the
first suspect for rollback.

**Proven live first (read-only script + screenshots, build 14189, de_mirage):**
- Row-major projection: the point 1000 units along the view angles lands exactly on (960, 540); up = up the screen,
  right = right, behind = negative w. Gives the Phase 5 angle convention too (pitch positive = down).
- **Bones:** scene node + `m_modelState` (0x140) + 0x80 → 32-byte bones. The joint indices differ from the commonly
  published ones (bone 27 is a look-at point 1000 units ahead; the legs are 17-22), so they were mapped from live
  positions in each bot's own frame, on CT and T models.
- Spotted-by masks all read 0: no bot was in line of sight (matches the screen and the radar). The bit numbering
  stays unverified until a bot is in plain view.

**Built**
- `maths/projection` (`world_to_screen`), `maths/skeleton` (indices, links, `project_skeleton`), `color.h`,
  `render/primitives.h`, `render/painter`, `settings/settings.h` (overlay + ESP; `config::Range`),
  `game/bones`, `game/visibility`, `features/esp` (`build_esp` and helpers), the ESP page (every option, colour
  pickers with opacity), `app/frame` (snapshot every frame while the ESP is on, ESP under the HUD, `active.esp`).
- Snapshot: `spotted_by_mask`, `bones`, `head_position()`, `slot()`.
- `--diag` section 6 "Match reads": CGlobalVars sanity and the bones on every alive player (48 checks in a match).
- Tests: projection (synthetic camera + live matrix), skeleton, bones, visibility, colour, settings, ESP (box
  geometry, team modes, skips, visibility colours, labels, styles). 97 cases / 1728 assertions.

**Problems and fixes**
- The "transposed matrix is wrong" test first compared the crosshair point, which a transposed matrix also puts near
  the centre (everything collapses there). It now uses a bot's feet, which don't project at all transposed.
- `Set-Content -Encoding utf8` (PowerShell 5.1) added a BOM to `main.cpp`; stripped.

**Verified here:** Debug and Release build with zero warnings; tests 97/97 in both; `--diag` all 48 checks OK
(bones: 20 of 20 players, head 52-60 units up). With every ESP option switched on in a scratch build (reverted, not
committed), screenshots over the live game showed boxes, skeletons, head circles, health, names, weapons, distances
and snaplines on all 19 bots, the skeletons inside their boxes; the ESP page rendered correctly. Read time with
bones: ~0.37 ms per snapshot.
**Not exercised in-game:** the visible colour (no bot in line of sight), SCOPED (bots had pistols), moving or dead
bots, other resolutions, a map change.

## 2026-10-07: Phase 5, aimbot + triggerbot

Started in autonomous mode the night before; the session ran out of context mid-phase without a `HANDOFF.md`. This
session picked up from the uncommitted tree (angles, aimbot, targeting, writes, settings and a key list existed; the
triggerbot, the frame wiring, the pages, the tests and the project entries didn't, and `esp_page.cpp` still used the
old `esp.team_mode`, so the tree didn't build). The user switched autonomous mode **off** partway through.

**User feedback on Phases 3-4:** everything works except the ESP head circle, which sat on the neck / shoulders.

**Head fix (proven live, build 14189):** bone 6 is the head joint at the base of the skull (~4 units below the eyes);
projected onto a screenshot of a bot in plain view it lands on the jaw. Bone 7 sits at eye height, 4.6 units forward,
inside the head. The head circle (now radius 6.5), head aim and the triggerbot's head check use bone 7
(`maths::bone::kHeadCentre`); the skeleton keeps bone 6. No offset changed.

**Proven live along the way**
- Spotted-by bit: with one bot in view, only its mask had bit 0 (our slot) set. Phase 4's open item is closed.
- `m_iIDEntIndex` = the target pawn's entity index (211 on the head, -1 fifteen degrees off). View restored.
- View-angle writes stick (40/40 at 100 ms and 10 ms intervals).

**Built**
- `maths/angles` (normalize, calc_aim_angles, angular distance, frame-rate-independent smoothing, step_towards,
  forward, distance_to_ray), `features/targeting` (shared enemy / live / visible / distance checks, now also used by
  the ESP), `features/aimbot` (aim point head / body / nearest, candidates, priority, compute_aim, FOV circle),
  `features/triggerbot` (target + block reasons, the firing state machine: reaction, single / burst / hold, shot
  delay), `features/activation` (hold / toggle), `game/writes` (button state, view angles), `input/keys` (the keys a
  feature can use until Phase 7).
- Settings: `GeneralSettings::team_mode` (shared by every feature), `AimbotSettings`, `TriggerbotSettings`.
- Pages: Aimbot and Triggerbot with every option and a live status line; `ui/pages/controls` shared by the feature
  pages (team mode on each).
- `app/frame`: polls the aim / trigger keys, aims and fires only while the game is in front and the menu is closed,
  writes attack only on a change, lets go of attack when the overlay hides or the tool exits; FOV circle; watermark
  lists Aimbot / Triggerbot. The overlay opens a read-write handle; `--diag` and `--live` stay read-only.
- Snapshot: `pawn_index`, `LocalState` (crosshair entity, flash alpha, view angles). Two schema fields from the dump
  (`m_flFlashOverlayAlpha`, `m_flFlashMaxAlpha`), checked by `--diag` (50 checks).
- Tests: angles, aimbot, triggerbot (state machine timings for each fire mode), writes, settings, head circle on bone
  7; `helpers/fake_game.h`. 125 cases / 2143 assertions.

**Problems and fixes**
- A Python heredoc edit of `external.vcxproj` turned `features\aimbot` into `features<BEL>imbot` (the known backslash
  collapse); the file was restored with git and edited with the Edit tool.

**Verified here:** Debug and Release build with zero warnings; tests 125/125 in both; `--diag` all 50 checks OK.
**Not verified (needs the user in-game):** the aimbot and triggerbot running inside the overlay (aiming, smoothing
feel, FOV circle size, every triggerbot option), the pages' layout.
**User check (2026-10-07):** aimbot, triggerbot and the fixed head circle all good in-game; approved for commit.
