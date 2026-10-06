#pragma once

// Non-offset constants and tunables. Offsets live in game/offsets.h and game/schema.h, never here.
//
// PURE: no <Windows.h>.

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace config
{
static_assert(sizeof(void*) == 8, "x64 only");

// --- Branding ------------------------------------------------------------------------------------------------------
inline constexpr char kAppName[] = "External Cheat";
inline constexpr char kAppAuthor[] = "by BigH";
inline constexpr char kAppEdition[] = "External";
inline constexpr wchar_t kOverlayClassName[] = L"ExternalCheatOverlay";
inline constexpr wchar_t kOverlayTitle[] = L"External Cheat - overlay";

// --- Pointer sanity ------------------------------------------------------------------------------------------------
// User-mode address range of an x64 Windows process. The first 64 KiB are never mapped.
inline constexpr std::uintptr_t kMinValidPointer = 0x10000;
inline constexpr std::uintptr_t kMaxValidPointer = 0x7FFFFFFFFFFF;
inline constexpr std::size_t kPageSize = 0x1000;

// --- The game and the modules we read from -------------------------------------------------------------------------
inline constexpr std::wstring_view kGameExe = L"cs2.exe";
inline constexpr std::wstring_view kClientModule = L"client.dll";
inline constexpr std::wstring_view kEngineModule = L"engine2.dll";
inline constexpr std::wstring_view kSchemaSystemModule = L"schemasystem.dll";
inline constexpr std::wstring_view kInputSystemModule = L"inputsystem.dll";
// The schema system's names for the client: its type scope, and the module name inside each class's info.
inline constexpr std::string_view kClientTypeScope = "client.dll";
inline constexpr std::string_view kClientSchemaModule = "client";

// --- Startup diagnostic (Phase 2) ----------------------------------------------------------------------------------
// `cs2_external.exe --diag` prints the offset diagnostic and exits (no overlay): the check to run after a CS2 update.
inline constexpr std::string_view kDiagnoseFlag = "--diag";
inline constexpr std::size_t kRemoteCopyChunk = 0x100000; // copying a module: 1 MiB per read, page by page on failure
inline constexpr std::size_t kMaxSignatureHits = 16;       // more matches than this = the pattern is useless
inline constexpr std::size_t kMaxInterfaces = 512;         // InterfaceReg list walk limit (guards against a loop)
inline constexpr std::size_t kMaxSchemaFields = 4096;      // per class (C_CSPlayerPawn has 104)
inline constexpr std::size_t kMaxNameLength = 256;         // interface, class and field names

// --- Keys ----------------------------------------------------------------------------------------------------------
// Temporary hard-coded menu key until the Phase 7 keybind engine. Virtual-key code VK_INSERT (no <Windows.h> here).
inline constexpr std::uint32_t kMenuToggleKey = 0x2D;
inline constexpr char kMenuToggleKeyName[] = "INSERT";

// --- Menu (ImGui) --------------------------------------------------------------------------------------------------
inline constexpr float kMenuWidth = 920.0f;  // initial size in pixels (shrunk to fit small game windows)
inline constexpr float kMenuHeight = 620.0f;
inline constexpr float kMenuMinWidth = 680.0f; // the menu can be resized, not below this
inline constexpr float kMenuMinHeight = 440.0f;
inline constexpr float kMenuMaxScreenFraction = 0.92f;
inline constexpr float kSidebarWidth = 190.0f;
inline constexpr float kHeaderHeight = 60.0f;
inline constexpr float kFontSize = 17.0f;      // base UI font size in pixels
inline constexpr float kTitleFontSize = 22.0f; // header title
inline constexpr float kPageTitleFontSize = 22.0f;
inline constexpr float kRowLabelWidth = 170.0f; // the label column of "label: value" rows
inline constexpr float kHeaderLogoSize = 38.0f; // the logo in the menu header
inline constexpr float kHomeLogoSize = 96.0f;   // the logo on the Home page

// --- Overlay -------------------------------------------------------------------------------------------------------
inline constexpr float kWatermarkMargin = 10.0f;   // from the top-left corner of the game's client area
inline constexpr float kFrameOutlineThickness = 2.0f;

// --- Timing --------------------------------------------------------------------------------------------------------
inline constexpr std::uint32_t kStatusIntervalMs = 250;     // match status read on the Home page (~4 Hz)
inline constexpr std::uint32_t kHiddenPollIntervalMs = 100; // overlay hidden: how often to look at the game again
inline constexpr std::uint32_t kShutdownWaitMs = 3000;      // console closed: how long Windows waits for our cleanup
} // namespace config
