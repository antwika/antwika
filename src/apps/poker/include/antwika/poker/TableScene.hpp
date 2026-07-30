#pragma once

#include <cstdint>

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/holdem/Card.hpp>

#include "antwika/poker/SeatSnapshot.hpp"
#include "antwika/poker/TableSnapshot.hpp"

namespace antwika::poker
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;

    /**
     * @brief Draws a table snapshot: the felt, the board, the pot and one
     * row per seat.
     *
     * Stateless and deterministic on purpose. The same canvas and the
     * same snapshot always produce the same drawing calls in the same
     * order, which is what makes the picture assertable against a mock
     * renderer instead of having to be looked at.
     */
    class TableScene final
    {
    public:
        /**
         * @brief Draw one frame.
         * @param renderer Receives the drawing calls.
         * @param canvas The size of the area being drawn into.
         * @param snapshot What to draw.
         */
        void draw(
            IRenderer &renderer,
            Size canvas,
            const TableSnapshot &snapshot) const;

    private:
        void drawHeader(
            IRenderer &renderer,
            Size canvas,
            const TableSnapshot &snapshot,
            std::uint32_t scale) const;

        void drawBoard(
            IRenderer &renderer,
            Rect area,
            const TableSnapshot &snapshot,
            std::uint32_t scale) const;

        void drawSeat(
            IRenderer &renderer,
            Rect box,
            const SeatSnapshot &seat,
            Chips largestStack,
            bool showButton,
            std::uint32_t scale) const;

        void drawCard(
            IRenderer &renderer,
            Rect box,
            holdem::Card card,
            std::uint32_t scale) const;
    };

} // namespace antwika::poker
