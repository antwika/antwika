#include "antwika/game/OverlayField.hpp"

#include <algorithm>
#include <cstddef>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/HousingQuery.hpp"
#include "antwika/game/ResourceColour.hpp"
#include "antwika/game/Store.hpp"

namespace antwika::game
{

    namespace
    {
        // What the top of the desirability scale is taken to be.
        // The field has no ceiling of its own, being a sum.
        // So one is chosen here rather than read off anything.
        // Two markets and a doctor is about as pleasant as a cell gets.
        constexpr std::int32_t kPleasantEnough = 6;

        // What the normal view is coloured, which nothing ever paints.
        // Its field is empty, so this is answered and never used.
        constexpr Color kNothing{
            .red = 255, .green = 255, .blue = 255};

        constexpr Color kDesirableInk{
            .red = 132, .green = 214, .blue = 160};

        // Nothing divides by a full of zero, and these are why.
        // Both are the only ceilings asShare() is ever handed.
        // Stated here rather than guarded for.
        // A guard on a constant is a branch no test could take.
        static_assert(kCoverageFull > 0);
        static_assert(kStockCapacity > 0);
        static_assert(kStoreCapacity > 0);
        static_assert(kPleasantEnough > 0);
        static_assert(kMaxRisk > 0);

        [[nodiscard]] std::int32_t asShare(
            std::int32_t held, std::int32_t full) noexcept
        {
            if (held <= 0)
            {
                return 0;
            }

            return std::min(std::int32_t{100}, held * 100 / full);
        }

        // Every cell of a block, kept inside the extent.
        // A block is the smallest thing any of these numbers is true of.
        void paintBlock(
            OverlayField &field,
            Cell origin,
            Footprint footprint,
            GridExtent extent,
            std::int32_t share)
        {
            for (std::int32_t dy = 0; dy < footprint.height; ++dy)
            {
                for (std::int32_t dx = 0; dx < footprint.width; ++dx)
                {
                    const Cell on{.x = origin.x + dx, .y = origin.y + dy};

                    if (extent.contains(on))
                    {
                        field[on] = share;
                    }
                }
            }
        }
    } // namespace

    OverlayField overlayFieldOf(
        const World &world,
        MapView view,
        const DesirabilityField &desirability,
        GridExtent extent)
    {
        OverlayField field;

        // The city itself, with nothing painted over it.
        // Which is why this is the empty answer rather than an arm.
        if (view == MapView::Normal)
        {
            return field;
        }

        if (view == MapView::Desirability)
        {
            for (const auto &[cell, amount] : desirability)
            {
                const auto share = asShare(amount, kPleasantEnough);

                if (share > 0 && extent.contains(cell))
                {
                    field[cell] = share;
                }
            }

            return field;
        }

        // The two risk views, off the building's own risks.
        // Out of kMaxRisk, so a strong tint is a building near its end.
        // The risks answer to no service any more -- see Building.
        if (view == MapView::Fire || view == MapView::Damage)
        {
            for (const auto entity : world.view<Building, Cell>())
            {
                const auto building = world.get<Building>(entity);
                const auto share = asShare(
                    view == MapView::Fire ? building.fireRisk
                                          : building.collapseRisk,
                    kMaxRisk);

                if (share <= 0)
                {
                    continue;
                }

                paintBlock(
                    field,
                    world.get<Cell>(entity),
                    footprintOf(building.kind),
                    extent,
                    share);
            }

            return field;
        }

        const auto service = mapViewService(view);
        const auto resource = mapViewResource(view);

        for (const auto entity : world.view<Building, Cell>())
        {
            const auto kind = world.get<Building>(entity).kind;

            // Whichever of the two this view is, and never both.
            // MapView.hpp's static_assert is what says so.
            // The shelf is the building's own -- stockCapacityAt().
            // A house's grows with its level.
            const auto share = service.has_value()
                ? asShare(
                      coverageOf(world, entity, *service), kCoverageFull)
                : asShare(
                      stockOf(world, entity, *resource),
                      stockCapacityAt(world, entity, kind));

            if (share <= 0)
            {
                continue;
            }

            paintBlock(
                field,
                world.get<Cell>(entity),
                footprintOf(kind),
                extent,
                share);
        }

        return field;
        // The excluded line is the local field's unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

    Color overlayColour(MapView view) noexcept
    {
        // The risk inks, named once beside serviceColour().
        // So a risk line and a map of that risk agree.
        if (view == MapView::Fire)
        {
            return kFireRiskInk;
        }

        if (view == MapView::Damage)
        {
            return kCollapseRiskInk;
        }

        const auto service = mapViewService(view);

        // Read off serviceColour() rather than named a second time.
        // So an amount line and a map of that coverage agree.
        if (service.has_value())
        {
            return serviceColour(*service);
        }

        const auto resource = mapViewResource(view);

        if (resource.has_value())
        {
            return resourceColour(*resource);
        }

        return view == MapView::Desirability ? kDesirableInk : kNothing;
    }

} // namespace antwika::game
