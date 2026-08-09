#include "antwika/tower_defence/Wave.hpp"

#include <cstddef>
#include <utility>

namespace antwika::tower_defence
{

    namespace
    {
        void shuffle(std::vector<MobKind> &order, IRng &rng)
        {
            for (std::size_t i = order.size(); i > 1; --i)
            {
                const auto j = static_cast<std::size_t>(rng.next() % i);
                std::swap(order[i - 1], order[j]);
            }
        }
    }

    std::uint32_t waveSize(const Wave &wave)
    {
        std::uint32_t total = 0;
        for (const WaveEntry &entry : wave.entries)
        {
            total += entry.count;
        }
        return total;
    }

    std::vector<WaveRelease> planWaves(
        const std::vector<Wave> &waves, IRng &rng)
    {
        std::vector<WaveRelease> planned;
        planned.reserve(waves.size());

        for (const Wave &wave : waves)
        {
            std::vector<MobKind> order;
            order.reserve(waveSize(wave));
            for (const WaveEntry &entry : wave.entries)
            {
                for (std::uint32_t made = 0; made < entry.count; ++made)
                {
                    order.push_back(entry.kind);
                }
            }

            shuffle(order, rng);

            planned.push_back(WaveRelease{ // GCOVR_EXCL_LINE
                .order = std::move(order),
                .spawnPeriodTicks = wave.spawnPeriodTicks,
                .gapTicks = wave.gapTicks});
        }

        return planned;
    } // GCOVR_EXCL_LINE

}
