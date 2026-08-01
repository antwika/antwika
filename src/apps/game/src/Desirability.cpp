#include "antwika/game/Desirability.hpp"

#include <algorithm>
#include <cstdint>

#include "antwika/game/Building.hpp"
#include "antwika/game/Footprint.hpp"

namespace antwika::game
{

    namespace
    {
        // How far outside a block a cell is, in Chebyshev cells.
        // Zero anywhere on the block itself.
        // So a storehouse reaches as far from each of its edges.
        // Exactly as a well does from its only one.
        [[nodiscard]] std::int32_t away(
            Cell origin, Footprint footprint, Cell cell) noexcept
        {
            const auto beforeX = origin.x - cell.x;
            const auto afterX = cell.x - (origin.x + footprint.width - 1);
            const auto beforeY = origin.y - cell.y;
            const auto afterY = cell.y - (origin.y + footprint.height - 1);

            const auto dx = std::max(0, std::max(beforeX, afterX));
            const auto dy = std::max(0, std::max(beforeY, afterY));

            return std::max(dx, dy);
        }
    } // namespace

    std::int32_t desirabilityFrom(
        DesirabilitySource source, std::int32_t distance) noexcept
    {
        return source.contribution * (source.radius - distance)
            / source.radius;
    }

    DesirabilityField desirabilityFieldOf(
        const World &world, GridExtent extent)
    {
        DesirabilityField field;

        for (const auto entity : world.view<Building, Cell>())
        {
            const auto kind = world.get<Building>(entity).kind;
            const auto source = desirabilityOf(kind);

            if (source.radius <= 0)
            {
                continue;
            }

            const auto origin = world.get<Cell>(entity);
            const auto footprint = footprintOf(kind);
            const auto reach = source.radius - 1;

            for (std::int32_t y = origin.y - reach;
                 y <= origin.y + footprint.height - 1 + reach;
                 ++y)
            {
                for (std::int32_t x = origin.x - reach;
                     x <= origin.x + footprint.width - 1 + reach;
                     ++x)
                {
                    const Cell cell{.x = x, .y = y};

                    if (!extent.contains(cell))
                    {
                        continue;
                    }

                    field[cell] += desirabilityFrom(
                        source, away(origin, footprint, cell));
                }
            }
        }

        // A cell nothing reaches is the same place to live.
        // As one everything reaching it cancelled out on.
        // So only one of the two is ever a member.
        std::erase_if(
            field, [](const auto &entry) { return entry.second == 0; });

        return field;
        // The excluded line is the local field's unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

    std::int32_t desirabilityAt(
        const DesirabilityField &field, Cell cell) noexcept
    {
        const auto found = field.find(cell);

        if (found == field.end())
        {
            return 0;
        }

        return found->second;
    }

} // namespace antwika::game
