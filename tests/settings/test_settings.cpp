#include <doctest.h>

#include "config.h"
#include "settings/settings.h"

TEST_CASE("Settings defaults: ESP off, sensible options, every number inside its range")
{
    const settings::Settings defaults;
    CHECK(defaults.overlay.watermark);
    CHECK_FALSE(defaults.overlay.frame_outline);

    const settings::EspSettings& esp = defaults.esp;
    CHECK_FALSE(esp.enabled); // nothing draws until the user turns it on
    CHECK(esp.team_mode == settings::TeamMode::teams);
    CHECK(esp.box);
    CHECK(esp.name);
    CHECK(esp.health_bar);
    CHECK(esp.visibility_colours);
    CHECK(config::kEspThickness.contains(esp.thickness));
    CHECK(config::kEspMaxDistance.contains(esp.max_distance));
    CHECK(esp.colours.enemy_visible.a == doctest::Approx(1.0f));
}

TEST_CASE("config::Range")
{
    constexpr config::Range<float> range{1.0f, 4.0f};
    CHECK(range.contains(1.0f));
    CHECK(range.contains(4.0f));
    CHECK_FALSE(range.contains(4.5f));
    CHECK(range.clamp(9.0f) == 4.0f);
    CHECK(range.clamp(-1.0f) == 1.0f);
    CHECK(range.clamp(2.5f) == 2.5f);
}
