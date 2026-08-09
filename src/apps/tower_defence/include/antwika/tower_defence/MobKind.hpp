#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace antwika::tower_defence
{

    enum class MobKind : std::uint8_t
    {
        Grunt = 0,

        Runner = 1,

        Brute = 2,

        Shielded = 3,
    };

    [[nodiscard]] constexpr MobKind enumBound(MobKind) noexcept
    {
        return MobKind::Shielded;
    }

    inline constexpr std::size_t kMobKindCount = 4;

    inline constexpr std::array<MobKind, kMobKindCount> kAllMobKinds{
        MobKind::Grunt,
        MobKind::Runner,
        MobKind::Brute,
        MobKind::Shielded};

    struct MobProfile final
    {
        std::uint32_t ticksPerCell = 1;

        std::int32_t health = 1;

        std::int32_t armour = 0;

        std::uint64_t reward = 0;

        [[nodiscard]] bool operator==(
            const MobProfile &other) const = default;
    };

    inline constexpr std::array<MobProfile, kMobKindCount>
        kDefaultMobProfiles{
            MobProfile{
                .ticksPerCell = 2, .health = 6, .armour = 0, .reward = 10},
            MobProfile{
                .ticksPerCell = 1, .health = 4, .armour = 0, .reward = 14},
            MobProfile{
                .ticksPerCell = 3,
                .health = 18,
                .armour = 0,
                .reward = 24},
            MobProfile{
                .ticksPerCell = 2,
                .health = 8,
                .armour = 1,
                .reward = 30}};

    [[nodiscard]] MobProfile profileOf(MobKind kind);

}
