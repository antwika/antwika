#include "antwika/game/GridScene.hpp"

#include <algorithm>
#include <cstdint>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/game/IsoProjection.hpp"

#include "DiamondSpans.hpp"

namespace antwika::game
{

    using antwika::gfx::Color;
    using antwika::gfx::Point;
    using antwika::gfx::Rect;
    using antwika::game::detail::fillDiamond;

    namespace
    {
        constexpr Color kSky{.red = 18, .green = 20, .blue = 28};
        constexpr Color kGround{.red = 44, .green = 58, .blue = 46};
        constexpr Color kLattice{.red = 66, .green = 84, .blue = 70};
        constexpr Color kPath{.red = 176, .green = 150, .blue = 96};

        // A walker's colour says which way it is facing.
        // A turn is then visible in a still frame, not only in motion.
        constexpr std::array<Color, kDirectionCount> kFacingColors{{
            {.red = 232, .green = 96, .blue = 96},
            {.red = 232, .green = 200, .blue = 96},
            {.red = 96, .green = 200, .blue = 232},
            {.red = 168, .green = 120, .blue = 232},
        }};

        [[nodiscard]] bool overlaps(Rect box, Size canvas) noexcept
        {
            const auto right =
                box.origin.x + static_cast<std::int32_t>(box.size.width);
            const auto bottom =
                box.origin.y + static_cast<std::int32_t>(box.size.height);

            return right >= 0 && bottom >= 0
                   && box.origin.x <= static_cast<std::int32_t>(canvas.width)
                   && box.origin.y
                          <= static_cast<std::int32_t>(canvas.height);
        }
    } // namespace

    bool GridScene::onCanvas(
        Cell cell, Size canvas, const SceneSnapshot &snapshot)
    {
        return overlaps(cellBounds(cell, snapshot.camera), canvas);
    }

    void GridScene::draw(
        IRenderer &renderer, Size canvas, const SceneSnapshot &snapshot) const
    {
        renderer.clear(kSky);

        // Diamonds tessellate, so one rectangle is the whole ground.
        // Only what differs from it needs drawing per cell.
        renderer.drawRect(
            Rect{.origin = {.x = 0, .y = 0}, .size = canvas}, kGround);

        drawLattice(renderer, canvas, snapshot);

        const auto halfWidth =
            static_cast<std::int32_t>(snapshot.camera.halfWidth());
        const auto halfHeight =
            static_cast<std::int32_t>(snapshot.camera.halfHeight());

        for (const auto cell : snapshot.paths)
        {
            if (!onCanvas(cell, canvas, snapshot))
            {
                continue;
            }

            fillDiamond(
                renderer,
                cellCentre(cell, snapshot.camera),
                halfWidth,
                halfHeight,
                kPath);
        }

        // Inset, so the path underneath still reads as a path.
        for (const auto &walker : snapshot.walkers)
        {
            if (!onCanvas(walker.at, canvas, snapshot))
            {
                continue;
            }

            fillDiamond(
                renderer,
                cellCentre(walker.at, snapshot.camera),
                halfWidth / 2,
                halfHeight / 2,
                kFacingColors[directionIndex(walker.facing)
                              % kDirectionCount]);
        }
    }

    void GridScene::drawLattice(
        IRenderer &renderer, Size canvas, const SceneSnapshot &snapshot) const
    {
        for (std::int32_t y = 0; y < snapshot.extent.height; ++y)
        {
            for (std::int32_t x = 0; x < snapshot.extent.width; ++x)
            {
                const Cell cell{.x = x, .y = y};

                if (!onCanvas(cell, canvas, snapshot))
                {
                    continue;
                }

                const auto top = cellToScreen(cell, snapshot.camera);
                const auto left = cellToScreen(
                    Cell{.x = x, .y = y + 1}, snapshot.camera);
                const auto right = cellToScreen(
                    Cell{.x = x + 1, .y = y}, snapshot.camera);

                // This cell's two upper edges only.
                // The lower two are its neighbours' upper ones.
                // So every shared edge is drawn once, not twice.
                renderer.drawLine(top, left, kLattice);
                renderer.drawLine(top, right, kLattice);
            }
        }
    }

} // namespace antwika::game
