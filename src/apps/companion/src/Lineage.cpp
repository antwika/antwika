#include "antwika/companion/Lineage.hpp"

#include "antwika/companion/SaveFormatError.hpp"

namespace antwika::companion
{

    Lineage::Lineage(const LineageMemory memory) : kept(memory)
    {
        if (kept.generation == 0)
        {
            throw SaveFormatError(
                "antwika::companion: a saved lineage is on its zeroth "
                "companion");
        }
    }

    void Lineage::record(const Tick ticks)
    {
        if (ticks <= kept.bestTicks)
        {
            return;
        }

        kept.bestTicks = ticks;
    }

    void Lineage::advance()
    {
        ++kept.generation;
    }

    LineageMemory Lineage::remember() const
    {
        return kept;
    }

    std::uint32_t Lineage::generation() const noexcept
    {
        return kept.generation;
    }

    Tick Lineage::bestTicks() const noexcept
    {
        return kept.bestTicks;
    }

}
