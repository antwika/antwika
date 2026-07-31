#include "antwika/tower_defence/BattleScene.hpp"

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/tower_defence/GridLayout.hpp"
#include "antwika/tower_defence/LevelTile.hpp"

namespace antwika::tower_defence
{

    using antwika::gfx::Color;
    using antwika::gfx::Rect;

    namespace
    {
        constexpr Color kBackground{
            .red = 18, .green = 22, .blue = 26, .alpha = 255};
        constexpr Color kGround{
            .red = 34, .green = 58, .blue = 40, .alpha = 255};
        constexpr Color kRoad{
            .red = 122, .green = 106, .blue = 74, .alpha = 255};
        constexpr Color kStart{
            .red = 86, .green = 168, .blue = 96, .alpha = 255};
        constexpr Color kEnd{
            .red = 190, .green = 78, .blue = 72, .alpha = 255};
        constexpr Color kTower{
            .red = 92, .green = 132, .blue = 198, .alpha = 255};
        constexpr Color kReach{
            .red = 92, .green = 132, .blue = 198, .alpha = 48};
        constexpr Color kMob{
            .red = 226, .green = 196, .blue = 84, .alpha = 255};

        Color colorFor(const Tile tile)
        {
            if (tile == Tile::Start)
            {
                return kStart;
            }
            if (tile == Tile::End)
            {
                return kEnd;
            }
            if (tile == Tile::Empty)
            {
                return kGround;
            }
            return kRoad;
        }

        // A cell's rectangle, shrunk in on every side.
        // Insetting is what leaves the ground showing between things.
        //
        // Saturating, because a cell can be one pixel across.
        // layoutFor() refuses a cell of zero pixels and no smaller size.
        // On unsigned widths a plain 1 - 2 is four billion.
        Rect inset(const Rect &rect, const std::uint32_t by)
        {
            const std::uint32_t taken = by * 2;
            return Rect{
                .origin = {
                    .x = rect.origin.x + static_cast<std::int32_t>(by),
                    .y = rect.origin.y + static_cast<std::int32_t>(by)},
                .size = {
                    .width = rect.size.width > taken
                        ? rect.size.width - taken
                        : 0,
                    .height = rect.size.height > taken
                        ? rect.size.height - taken
                        : 0}};
        }
    } // namespace

    void BattleScene::draw(
        IRenderer &renderer,
        const Size canvas,
        const BattleSnapshot &snapshot) const
    {
        renderer.clear(kBackground);

        const auto layout = layoutFor(
            canvas, snapshot.level.width, snapshot.level.height);
        if (!layout)
        {
            return;
        }

        for (std::uint32_t y = 0; y < snapshot.level.height; ++y)
        {
            for (std::uint32_t x = 0; x < snapshot.level.width; ++x)
            {
                const Cell cell{.x = x, .y = y};
                renderer.drawRect(
                    inset(cellRect(*layout, cell), 1),
                    colorFor(snapshot.level.at(cell)));
            }
        }

        // Reach first, so a tower is drawn on top of its own halo.
        const std::uint32_t radius =
            rangeRadius(snapshot.towerRangeSquared);
        for (const Cell &tower : snapshot.towers)
        {
            const Rect centre = cellRect(*layout, tower);
            const auto span =
                static_cast<std::int32_t>(radius * layout->cell);
            renderer.drawRect(
                Rect{
                    .origin = {
                        .x = centre.origin.x - span,
                        .y = centre.origin.y - span},
                    .size = {
                        .width = centre.size.width
                            + (radius * 2 * layout->cell),
                        .height = centre.size.height
                            + (radius * 2 * layout->cell)}},
                kReach);
        }

        for (const Cell &tower : snapshot.towers)
        {
            renderer.drawRect(
                inset(cellRect(*layout, tower), layout->cell / 6), kTower);
        }

        for (const Cell &mob : snapshot.mobs)
        {
            renderer.drawRect(
                inset(cellRect(*layout, mob), layout->cell / 3), kMob);
        }
    }

} // namespace antwika::tower_defence
