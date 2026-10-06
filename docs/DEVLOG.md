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
