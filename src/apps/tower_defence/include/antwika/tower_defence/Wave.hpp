#pragma once

#include <cstdint>
#include <vector>

#include <antwika/rng/IRng.hpp>

#include "antwika/tower_defence/MobKind.hpp"

namespace antwika::tower_defence
{

    using antwika::rng::IRng;

    struct WaveEntry final
    {
        MobKind kind = MobKind::Grunt;
        std::uint32_t count = 0;

        [[nodiscard]] bool operator==(const WaveEntry &) const = default;
    };

    struct Wave final
    {
        std::vector<WaveEntry> entries;

        std::uint64_t spawnPeriodTicks = 4;

        std::uint64_t gapTicks = 24;
    };

    struct WaveRelease final
    {
        std::vector<MobKind> order;

        std::uint64_t spawnPeriodTicks = 4;
        std::uint64_t gapTicks = 0;
    };

    [[nodiscard]] std::vector<WaveRelease> planWaves(
        const std::vector<Wave> &waves, IRng &rng);

    [[nodiscard]] std::uint32_t waveSize(const Wave &wave);

}
