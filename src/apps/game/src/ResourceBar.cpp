#include "antwika/game/ResourceBar.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/Walker.hpp"
#include "antwika/game/WalkerMotion.hpp"

namespace antwika::game
{

    namespace
    {
        using antwika::gfx::Point;
        using antwika::gfx::Size;

        // Nothing divides by a capacity of zero, and these are why.
        // Both are the only capacities barIn() is ever handed.
        // Stated here rather than guarded for.
        // A guard on a constant is a branch no test could take.
        static_assert(kStockCapacity > 0);
        static_assert(kWalkerLoad > 0);

        // A bar has to stay legible at the furthest zoom.
        // A whole tile is eight pixels across and four down there.
        constexpr std::int32_t kMinBarWidth = 2;
        constexpr std::int32_t kMinBarHeight = 4;

        constexpr Color kFoodFill{
            .red = 126, .green = 196, .blue = 84};

        constexpr Color kClayFill{
            .red = 178, .green = 122, .blue = 78};

        constexpr Color kPotteryFill{
            .red = 210, .green = 168, .blue = 96};

        // Derived from the camera rather than fixed.
        // A bar keeps its proportion to what it gauges at every zoom.
        [[nodiscard]] std::int32_t barWidth(const Camera &camera) noexcept
        {
            return std::max(
                kMinBarWidth,
                static_cast<std::int32_t>(camera.halfWidth() / 8));
        }

        [[nodiscard]] std::int32_t barHeight(const Camera &camera) noexcept
        {
            return std::max(
                kMinBarHeight,
                static_cast<std::int32_t>(camera.halfHeight()));
        }

        [[nodiscard]] std::int32_t barGap(const Camera &camera) noexcept
        {
            return std::max(1, barWidth(camera) / 2);
        }

        // The box a row of bars is laid out over, and how many it holds.
        // That box is always the one the sprite itself is drawn into.
        struct Row
        {
            Rect box;
            std::int32_t count = 1;
            const Camera &camera;
        };

        [[nodiscard]] ResourceBar barIn(
            const Row &row,
            std::int32_t slot,
            Resource resource,
            std::int32_t held,
            std::int32_t capacity)
        {
            const auto width = barWidth(row.camera);
            const auto height = barHeight(row.camera);
            const auto gap = barGap(row.camera);

            const auto span = row.count * width + (row.count - 1) * gap;

            // Centred on the sprite's box and sitting just above it.
            // Above rather than over, so no bar hides the art it gauges.
            const auto left = row.box.origin.x
                + (static_cast<std::int32_t>(row.box.size.width) - span) / 2
                + slot * (width + gap);
            const auto top = row.box.origin.y - height;

            // Clamped, so a bar never draws outside its own track.
            // Nothing in the simulation puts a stock out of range.
            // But a gauge that lies about its one number is worse.
            const auto clamped = std::min(std::max(held, 0), capacity);
            const auto filled = height * clamped / capacity;

            return ResourceBar{
                .resource = resource,
                .track =
                    {.origin = Point{.x = left, .y = top},
                     .size = Size{
                         .width = static_cast<std::uint32_t>(width),
                         .height = static_cast<std::uint32_t>(height)}},
                .fill = {
                    .origin = Point{.x = left, .y = top + height - filled},
                    .size = Size{
                        .width = static_cast<std::uint32_t>(width),
                        .height = static_cast<std::uint32_t>(filled)}}};
        }
    } // namespace

    Color resourceColour(Resource resource) noexcept
    {
        constexpr std::array<Color, kResourceCount> fills{
            kFoodFill,
            kClayFill,
            kPotteryFill};

        return fills[resourceIndex(resource) % kResourceCount];
    }

    std::vector<ResourceBar> buildingBars(
        const BuildingSprite &building, const Camera &camera)
    {
        // What a building depends on is what it consumes.
        // A source keeps stock nobody drains -- see BuildingSystem.
        // So a gauge on one would count something that never moves.
        if (!consumes(building.kind))
        {
            return {};
        }

        const Row row{
            .box = footprintBounds(
                building.at, footprintOf(building.kind), camera),
            .count = static_cast<std::int32_t>(kResourceCount),
            .camera = camera};

        std::vector<ResourceBar> bars;
        bars.reserve(kResourceCount);

        for (std::size_t slot = 0; slot < kResourceCount; ++slot)
        {
            bars.push_back(
                barIn(
                    row,
                    static_cast<std::int32_t>(slot),
                    kResources[slot],
                    building.stock[slot],
                    kStockCapacity));
        }

        return bars;
        // The excluded line is the local vector's unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

    std::vector<ResourceBar> walkerBars(
        const WalkerSprite &walker,
        const Camera &camera,
        Progress subTick)
    {
        const auto carries = carriedResource(walker.kind);

        // A service walker carries nothing and gauges nothing.
        if (!carries.has_value())
        {
            return {};
        }

        const Row row{
            .box = walkerBounds(walker, camera, subTick),
            .count = 1,
            .camera = camera};

        return {barIn(row, 0, *carries, walker.carried, kWalkerLoad)};
        // The excluded line is the returned vector's unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

} // namespace antwika::game
