# Offsets, signatures and schema fields

Every offset the code uses, where it came from, and how it was proven. The rules are in `CLAUDE.md` §7: never change
an offset without a diagnostic, keep the old value as a comment, flag it as **OFFSET CHANGED**.

## Dump provenance

All files in `docs/dumps/` are byte-for-byte copies of one a2x/cs2-dumper run by the user (full output kept outside
the repo at `C:\Users\Harry\Desktop\output`).

| File | Contents | CS2 build | Dumped (UTC) |
|---|---|---|---|
| `info.json` | build number + timestamp of the run | 14189 | 2026-10-06T16:04:45Z |
| `offsets.json` | 29 module globals across 5 modules | 14189 | same run |
| `client_dll.json` | 542 schema classes, 14 enums | 14189 | same run |
| `interfaces.json` | 111 interfaces across 31 modules | 14189 | same run |
| `buttons.json` | 16 button globals (client.dll) | 14189 | same run |

The dumper's own git commit isn't written to `info.json`, so build number + timestamp are the provenance.

## Module RVAs (`src/external/game/offsets.h`)

| Name | Module | Value (hex) | Value (dec, as dumped) | Used since | Proof |
|---|---|---|---|---|---|
| `dwLocalPlayerPawn` | client.dll | `0x2562808` | 39200776 | Phase 0 | 2026-10-06, build 14189, offline deathmatch vs bots: `client.dll` base `0x7FFD62930000` + `0x2562808` → `0x4DF564BF800` (user-space, 8-byte aligned, stable across two runs). Semantic check (health/team read from it) comes with Phase 3. |

## Schema fields (`src/external/game/schema.h`)

None yet (Phase 2).

## Signatures

None yet (Phase 2).

## Hand-found offsets

None yet. The bone array pointer is planned for after Phase 4 (see `CLAUDE.md` §7).

## Planned fields and buttons (not in code yet)

Looked up on 2026-10-06 for the feature plan (bunny hop, ESP scoped indicator, visibility heuristic, configurable
triggerbot), from the build-14189 dumps in `docs/dumps/`. Not used by any code yet: Phase 2 copies them into
`schema.h` / `offsets.h` and proves them with its diagnostic. Button value formats get an in-game check before use.

| Name | Where | Value (hex) | Source | For |
|---|---|---|---|---|
| `m_iHealth` | `C_BaseEntity` | `0x34C` | `client_dll.json` | ESP, aimbot, triggerbot |
| `m_fFlags` | `C_BaseEntity` | `0x3F4` | `client_dll.json` | bunny hop (bit 0 `FL_ONGROUND`), triggerbot "in air" |
| `m_vecVelocity` | `C_BaseEntity` | `0x430` | `client_dll.json` | bunny hop diagnostics |
| `m_bIsScoped` | `C_CSPlayerPawn` | `0x1EA0` | `client_dll.json` | ESP scoped indicator, triggerbot "only when scoped" |
| `m_entitySpottedState` | `C_CSPlayerPawn` | `0x1E88` | `client_dll.json` | visibility heuristic |
| `m_bSpottedByMask` | `EntitySpottedState_t` | `+0xC` | `client_dll.json` | visibility heuristic (bit per player slot) |
| `m_iIDEntIndex` | `C_CSPlayerPawn` | `0x36CC` | `client_dll.json` | triggerbot (entity under the crosshair) |
| `m_iShotsFired` | `C_CSPlayerPawn` | `0x1EB4` | `client_dll.json` | triggerbot burst counting |
| `m_flFlashDuration` | `C_CSPlayerPawnBase` | `0x1510` | `client_dll.json` | triggerbot "not while flashed" |
| `jump` | client.dll | `0x22324E0` (35857632) | `buttons.json` | bunny hop |
| `attack` | client.dll | `0x2231FD0` (35856336) | `buttons.json` | triggerbot |
