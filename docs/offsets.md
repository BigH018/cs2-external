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
