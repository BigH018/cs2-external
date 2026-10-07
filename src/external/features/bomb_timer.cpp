#include "features/bomb_timer.h"

#include <algorithm>
#include <format>
#include <initializer_list>

#include "config.h"
#include "features/esp.h"

namespace features
{
namespace
{
constexpr Color kBackground = Color::rgb(0x0E1220, 0.78f);
constexpr Color kBorder = Color::rgb(0x8CC4CF, 0.55f);
constexpr Color kBarBack{0.0f, 0.0f, 0.0f, 0.6f};
constexpr Color kText = Color::rgb(0xEDEBF7);
constexpr Color kDim = Color::rgb(0xEDEBF7, 0.65f);
constexpr Color kGood = Color::rgb(0x5CD65C);
constexpr Color kWarn = Color::rgb(0xF2D45C);
constexpr Color kBad = Color::rgb(0xF25C5C);
constexpr Color kDefuse = Color::rgb(0x5CA8F2);
constexpr float kRowGap = 4.0f;

// A vertical line across a bar at `fraction` of its width (the countdown bar's latest-defuse marks).
struct BarMark
{
    float fraction;
    Color colour;
};

Color verdict_colour(DefuseVerdict verdict) noexcept
{
    switch (verdict)
    {
    case DefuseVerdict::no_kit_needed: return kGood;
    case DefuseVerdict::kit_needed: return kWarn;
    case DefuseVerdict::too_late: break;
    }
    return kBad;
}

const char* verdict_text(DefuseVerdict verdict) noexcept
{
    switch (verdict)
    {
    case DefuseVerdict::no_kit_needed: return "Time to defuse without a kit";
    case DefuseVerdict::kit_needed: return "Defuse only with a kit";
    case DefuseVerdict::too_late: break;
    }
    return "Too late to defuse";
}

float fraction(float left, float total) noexcept
{
    return total > 0.0f ? std::clamp(left / total, 0.0f, 1.0f) : 0.0f;
}

// Rows top to bottom inside the panel; the background is added under them once their height is known.
class PanelWriter
{
public:
    PanelWriter(std::vector<render::Primitive>& out, float left, float top, float line_height)
        : out_(out), left_(left + config::kBombPanelPadding),
          right_(left + config::kBombPanelWidth - config::kBombPanelPadding), y_(top + config::kBombPanelPadding),
          line_height_(line_height)
    {
    }

    void text(const std::string& left, Color left_colour, const std::string& right = {}, Color right_colour = kText)
    {
        out_.push_back(render::Text{{left_, y_}, left, left_colour, render::TextAnchor::top_left});
        if (!right.empty())
        {
            out_.push_back(render::Text{{right_, y_}, right, right_colour, render::TextAnchor::top_right});
        }
        y_ += line_height_ + kRowGap;
    }

    void bar(float filled, Color colour, std::initializer_list<BarMark> marks = {})
    {
        const float end = left_ + (right_ - left_) * std::clamp(filled, 0.0f, 1.0f);
        out_.push_back(render::FilledRect{{left_, y_}, {right_, y_ + config::kBombBarHeight}, kBarBack});
        out_.push_back(render::FilledRect{{left_, y_}, {end, y_ + config::kBombBarHeight}, colour});
        for (const BarMark& mark : marks)
        {
            if (mark.fraction <= 0.0f || mark.fraction >= 1.0f)
            {
                continue;
            }
            const float x = left_ + (right_ - left_) * mark.fraction;
            const maths::Vec2 top{x, y_ - config::kBombMarkOverhang};
            const maths::Vec2 bottom{x, y_ + config::kBombBarHeight + config::kBombMarkOverhang};
            out_.push_back(render::Line{top, bottom, kBarBack, config::kBombMarkThickness + 2.0f});
            out_.push_back(render::Line{top, bottom, mark.colour, config::kBombMarkThickness});
        }
        y_ += config::kBombBarHeight + kRowGap;
    }

    // The bottom of the last row plus the padding (the gap after the last row stands in for part of it).
    [[nodiscard]] float bottom() const noexcept { return y_ - kRowGap + config::kBombPanelPadding; }

private:
    std::vector<render::Primitive>& out_;
    float left_;
    float right_;
    float y_;
    float line_height_;
};
} // namespace

DefuseVerdict defuse_verdict(float seconds_left) noexcept
{
    if (seconds_left > config::kDefuseSecondsNoKit)
    {
        return DefuseVerdict::no_kit_needed;
    }
    return seconds_left > config::kDefuseSecondsKit ? DefuseVerdict::kit_needed : DefuseVerdict::too_late;
}

std::optional<BombTimerInfo> bomb_timer_info(const game::GameSnapshot& game)
{
    if (!game.bomb)
    {
        return std::nullopt;
    }
    const game::PlantedBomb& bomb = *game.bomb;
    const float now = game.globals.curtime;

    BombTimerInfo info;
    info.site = bomb.site == 0 ? 'A' : bomb.site == 1 ? 'B' : '?';
    info.seconds_left = std::max(bomb.blow_time - now, 0.0f);
    info.fraction_left = fraction(info.seconds_left, bomb.timer_length);
    info.timer_length = bomb.timer_length;
    info.verdict = defuse_verdict(info.seconds_left);
    if (bomb.exploded)
    {
        info.phase = BombPhase::exploded;
    }
    else if (bomb.defused)
    {
        info.phase = BombPhase::defused;
    }
    else if (bomb.being_defused)
    {
        info.phase = BombPhase::defusing;
        info.defuse_left = std::max(bomb.defuse_end - now, 0.0f);
        info.defuse_fraction_left = fraction(info.defuse_left, bomb.defuse_length);
        info.defuse_in_time = bomb.defuse_end < bomb.blow_time;
        for (const game::PlayerSnapshot& player : game.players)
        {
            if (bomb.defuser_pawn != 0 && player.pawn == bomb.defuser_pawn)
            {
                info.defuser = display_name(player.name);
            }
        }
    }
    if (const game::PlayerSnapshot* local = game.local(); local != nullptr && local->pawn != 0)
    {
        info.distance_metres = maths::units_to_metres(local->origin.distance_to(bomb.position));
    }
    return info;
}

std::vector<render::Primitive> build_bomb_timer(const game::GameSnapshot& game,
                                                const settings::BombTimerSettings& settings, maths::Vec2 screen,
                                                float line_height)
{
    std::vector<render::Primitive> out;
    const auto info = settings.enabled ? bomb_timer_info(game) : std::nullopt;
    if (!info)
    {
        return out;
    }
    const float left = (screen.x - config::kBombPanelWidth) * 0.5f;
    const float top = config::kBombTimerTop.clamp(settings.top);
    out.push_back(render::FilledRect{}); // the background, filled in once the height is known
    PanelWriter panel(out, left, top, line_height);

    const std::string title = std::format("BOMB {}", info->site);
    switch (info->phase)
    {
    case BombPhase::exploded: panel.text(title + " EXPLODED", kBad); break;
    case BombPhase::defused: panel.text(title + " DEFUSED", kGood); break;
    case BombPhase::ticking:
    case BombPhase::defusing:
    {
        const Color colour = verdict_colour(info->verdict);
        panel.text(title, colour, std::format("{:.1f} s", info->seconds_left), colour);
        // The latest moments a defuse can start: without a kit (10 s left), with one (5 s left).
        panel.bar(info->fraction_left, colour,
                  {BarMark{fraction(config::kDefuseSecondsNoKit, info->timer_length), kWarn},
                   BarMark{fraction(config::kDefuseSecondsKit, info->timer_length), kBad}});
        if (info->phase == BombPhase::defusing)
        {
            const std::string who = info->defuser.empty() ? "Defusing" : "Defusing: " + info->defuser;
            panel.text(who, kDefuse, std::format("{:.1f} s", info->defuse_left), info->defuse_in_time ? kGood : kBad);
            panel.bar(info->defuse_fraction_left, kDefuse);
            panel.text(info->defuse_in_time ? "Defuse will make it" : "Defuse is too late",
                       info->defuse_in_time ? kGood : kBad);
        }
        else if (settings.defuse_hint)
        {
            panel.text(verdict_text(info->verdict), colour);
        }
        break;
    }
    }
    if (settings.distance && info->distance_metres)
    {
        panel.text(std::format("{:.0f} m from you", *info->distance_metres), kDim);
    }

    const maths::Vec2 min{left, top};
    const maths::Vec2 max{left + config::kBombPanelWidth, panel.bottom()};
    out.front() = render::FilledRect{min, max, kBackground};
    out.insert(out.begin() + 1, render::Rect{min, max, kBorder, 1.5f});
    return out;
}
} // namespace features
