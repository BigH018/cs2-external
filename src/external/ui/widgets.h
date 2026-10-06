#pragma once

// Small building blocks shared by the menu pages. Every size goes through ui::scaled(). Phase 10 adds the richer set
// from the AC project (switches, chips, two-column cards); only what the Phase 1 pages use is here.

#include <initializer_list>

#include <imgui.h>

#include "ui/theme.h"

namespace ui::widgets
{
// Page title (bold, large) with a dimmed one-line subtitle under it.
void page_header(const Fonts& fonts, const char* title, const char* subtitle);

// A titled card (rounded surface box) that grows to fit its contents. Always pair with card_end().
// The title doubles as the card's ID, so two cards on one page need different titles.
void card_begin(const Fonts& fonts, const char* title);
void card_end();

// Dimmed, wrapped explanatory text.
void hint(const char* text);

// A dimmed "(?)" on the current line; hovering it shows `text` (wrapped).
void help_marker(const char* text);

// "label ......... value" on one line (label dimmed, value at the row-label column).
void info_row(const char* label, const char* value);
void info_row(const char* label, const char* value, const ImVec4& value_colour);

// A one-line message in accent colour, or warning colour.
void notice(const char* text, bool warning);

// A small rounded label ("In match", "External"), filled with `colour` at low opacity and outlined.
void pill(const char* text, const ImVec4& colour);

// A card for a page that isn't built yet: "Arrives in <phase>" and a bullet list of what it will hold.
void planned_card(const Fonts& fonts, const char* phase, std::initializer_list<const char*> features);

// An image with rounded corners at the cursor (the logo). Only reserves the space if `texture` is null.
void image_rounded(ImTextureData* texture, float size, float rounding);
} // namespace ui::widgets
