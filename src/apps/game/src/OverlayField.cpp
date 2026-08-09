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
        constexpr std::int32_t kPleasantEnough = 6;

        constexpr Color kNothing{
            .red = 255, .green = 255, .blue = 255};

        constexpr Color kDesirableInk{
            .red = 132, .green = 214, .blue = 160};

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
    }

    OverlayField overlayFieldOf(
        const World &world,
        MapView view,
        const DesirabilityField &desirability,
        GridExtent extent)
    {
        OverlayField field;

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
    } // GCOVR_EXCL_LINE

    Color overlayColour(MapView view) noexcept
    {
        if (view == MapView::Fire)
        {
            return kFireRiskInk;
        }

        if (view == MapView::Damage)
        {
            return kCollapseRiskInk;
        }

        const auto service = mapViewService(view);

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

}
