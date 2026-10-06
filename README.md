# CS2 External (offline learning project)

A personal learning project in reverse engineering and low-level Windows programming: a user-mode tool that reads
Counter-Strike 2's memory from outside the process with `ReadProcessMemory`.

**Offline only.** CS2 is launched with `-insecure` and played against bots on the author's own PC. The tool is never run
against a VAC-secured server, never distributed, and contains no evasion of any kind.

The full educational README comes in Phase 11. Until then, see `CLAUDE.md` for scope, architecture and the roadmap.

## Build

Visual Studio 2026 (MSVC toolset v143), x64 only:

```powershell
$msb = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"
& $msb cs2-external.sln /m /nologo /v:minimal /p:Configuration=Release /p:Platform=x64
bin\Release\tests.exe
bin\Release\cs2_external.exe   # with CS2 running under -insecure
```
