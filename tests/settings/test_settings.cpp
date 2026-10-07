#include <cstddef>

#include <doctest.h>

#include "config.h"
#include "input/keys.h"
#include "settings/settings.h"

TEST_CASE("Settings defaults: ESP off, sensible options, every number inside its range")
{
    const settings::Settings defaults;
    CHECK(defaults.overlay.watermark);
    CHECK_FALSE(defaults.overlay.frame_outline);

    CHECK(defaults.general.team_mode == settings::TeamMode::teams);

    const settings::EspSettings& esp = defaults.esp;
    CHECK_FALSE(esp.enabled); // nothing draws until the user turns it on
    CHECK(esp.box);
    CHECK(esp.name);
    CHECK(esp.health_bar);
    CHECK(esp.visibility_colours);
    CHECK(config::kEspThickness.contains(esp.thickness));
    CHECK(config::kMaxDistance.contains(esp.max_distance));
    CHECK(esp.colours.enemy_visible.a == doctest::Approx(1.0f));
}

TEST_CASE("Settings defaults: aimbot and triggerbot off, every number inside its range, keys bindable")
{
    const settings::Settings defaults;
    const settings::AimbotSettings& aim = defaults.aimbot;
    CHECK_FALSE(aim.enabled); // nothing writes to the game until the user turns it on
    CHECK(aim.mode == settings::BindMode::hold);
    CHECK(aim.team_check);
    CHECK(config::kAimFov.contains(aim.fov));
    CHECK(config::kAimSmoothing.contains(aim.smoothing));
    CHECK(config::kMaxDistance.contains(aim.max_distance));
    CHECK(input::key_index(aim.key) >= 0);

    const settings::TriggerbotSettings& trigger = defaults.triggerbot;
    CHECK_FALSE(trigger.enabled);
    CHECK(trigger.team_check);
    CHECK(config::kTriggerReaction.contains(trigger.reaction_ms));
    CHECK(config::kTriggerShotDelay.contains(trigger.shot_delay_ms));
    CHECK(config::kTriggerBurst.contains(trigger.burst_shots));
    CHECK(config::kMaxDistance.contains(trigger.max_distance));
    CHECK(input::key_index(trigger.key) >= 0);
}

TEST_CASE("Settings defaults: radar off, every number inside its range")
{
    const settings::RadarSettings radar = settings::Settings{}.radar;
    CHECK_FALSE(radar.enabled);
    CHECK(radar.corner == settings::RadarCorner::top_right); // clear of the watermark and the game's radar
    CHECK(config::kRadarSize.contains(radar.size));
    CHECK(config::kRadarRange.contains(radar.range));
    CHECK(config::kRadarDotSize.contains(radar.dot_size));
    CHECK(radar.colours.background.a < 1.0f); // see-through
}

TEST_CASE("Settings defaults: bomb timer off, inside its range")
{
    const settings::BombTimerSettings timer = settings::Settings{}.bomb_timer;
    CHECK_FALSE(timer.enabled);
    CHECK(config::kBombTimerTop.contains(timer.top));
    CHECK(timer.defuse_hint);
    CHECK(timer.distance);
}

TEST_CASE("input::kBindableKeys: names and indices")
{
    CHECK(input::key_name(0x01) == "Mouse 1");
    CHECK(input::key_name(0x05) == "Mouse 4");
    CHECK(input::key_name(0xFF) == "?");
    CHECK(input::key_index(0x01) == 0);
    CHECK(input::key_index(0xFF) == -1);
    for (std::size_t i = 0; i < input::kBindableKeys.size(); ++i)
    {
        CHECK(input::key_index(input::kBindableKeys[i].vk) == static_cast<int>(i)); // no duplicates
    }
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
