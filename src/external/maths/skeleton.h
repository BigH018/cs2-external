#pragma once

// The player skeleton: which bone index is which joint, and which joints a skeleton drawing connects.
//
// The indices are not the ones public CS2 projects list (those have the legs at 22-27; in build 14189 bone 27 is a
// look-at point 1000 units in front of the face). They were mapped 2026-10-06 from the live bone positions of CT and T
// bots in each bot's own frame (forward / left / up from its feet): e.g. head 6 at ~59 up, left hand 11 at ~17
// forward, right foot 22 at ~4 up. Re-check after an update that changes player models.
//
// PURE: no <Windows.h>.

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "maths/projection.h"
#include "maths/vec.h"

namespace maths
{
namespace bone
{
inline constexpr std::uint8_t kRoot = 0; // at the feet
inline constexpr std::uint8_t kPelvis = 1;
inline constexpr std::uint8_t kSpine1 = 2;
inline constexpr std::uint8_t kSpine2 = 3;
inline constexpr std::uint8_t kSpine3 = 4;
inline constexpr std::uint8_t kNeck = 5;
inline constexpr std::uint8_t kHead = 6;
inline constexpr std::uint8_t kLeftShoulder = 9;
inline constexpr std::uint8_t kLeftElbow = 10;
inline constexpr std::uint8_t kLeftHand = 11;
inline constexpr std::uint8_t kRightShoulder = 13;
inline constexpr std::uint8_t kRightElbow = 14;
inline constexpr std::uint8_t kRightHand = 15;
inline constexpr std::uint8_t kLeftHip = 17;
inline constexpr std::uint8_t kLeftKnee = 18;
inline constexpr std::uint8_t kLeftFoot = 19;
inline constexpr std::uint8_t kRightHip = 20;
inline constexpr std::uint8_t kRightKnee = 21;
inline constexpr std::uint8_t kRightFoot = 22;
} // namespace bone

// Bones 0..22 are read: every joint above.
inline constexpr std::size_t kBoneCount = 23;
using Bones = std::array<Vec3, kBoneCount>;

struct BoneLink
{
    std::uint8_t from;
    std::uint8_t to;
};

// The lines of a skeleton drawing: spine, arms from the neck, legs from the pelvis.
inline constexpr std::array kSkeletonLinks{
    BoneLink{bone::kHead, bone::kNeck},           BoneLink{bone::kNeck, bone::kSpine3},
    BoneLink{bone::kSpine3, bone::kSpine2},       BoneLink{bone::kSpine2, bone::kSpine1},
    BoneLink{bone::kSpine1, bone::kPelvis},       BoneLink{bone::kNeck, bone::kLeftShoulder},
    BoneLink{bone::kLeftShoulder, bone::kLeftElbow}, BoneLink{bone::kLeftElbow, bone::kLeftHand},
    BoneLink{bone::kNeck, bone::kRightShoulder},  BoneLink{bone::kRightShoulder, bone::kRightElbow},
    BoneLink{bone::kRightElbow, bone::kRightHand}, BoneLink{bone::kPelvis, bone::kLeftHip},
    BoneLink{bone::kLeftHip, bone::kLeftKnee},    BoneLink{bone::kLeftKnee, bone::kLeftFoot},
    BoneLink{bone::kPelvis, bone::kRightHip},     BoneLink{bone::kRightHip, bone::kRightKnee},
    BoneLink{bone::kRightKnee, bone::kRightFoot},
};

struct ScreenLine
{
    Vec2 from;
    Vec2 to;
};

// The skeleton's lines on screen. A link with either end behind the camera is left out.
[[nodiscard]] std::vector<ScreenLine> project_skeleton(const ViewMatrix& view, const Bones& bones, Vec2 screen);
} // namespace maths
