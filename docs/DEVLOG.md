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
