#pragma once

#include <cstdint>

#include <antwika/time/Tick.hpp>

namespace antwika::companion
{

    using antwika::time::Tick;

    struct LineageMemory final
    {
        std::uint32_t generation = 1;

        Tick bestTicks = 0;

        [[nodiscard]] bool operator==(const LineageMemory &other) const
            = default;
    };

    class Lineage final
    {
    public:
        explicit Lineage(LineageMemory memory = {});

        void record(Tick ticks);

        void advance();

        [[nodiscard]] LineageMemory remember() const;

        [[nodiscard]] std::uint32_t generation() const noexcept;

        [[nodiscard]] Tick bestTicks() const noexcept;

    private:
        LineageMemory kept;
    };

}
