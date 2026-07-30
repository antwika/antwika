#pragma once

#include <cstdint>
#include <span>

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/holdem/Card.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Frame.hpp>

#include "antwika/poker/SeatSnapshot.hpp"
#include "antwika/poker/TableSnapshot.hpp"

namespace antwika::poker
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;
    using antwika::ui::Context;
    using antwika::ui::Frame;

    /**
     * @brief Draws a table snapshot: the felt, the board, the pot and one
     * row per seat.
     *
     * Stateless and deterministic on purpose. The same canvas and the
     * same snapshot always produce the same picture, which is what makes
     * it assertable without anything having to be looked at.
     *
     * Every measurement it needs -- the glyph scale, the insets, the
     * centring, the space a row has left -- comes out of antwika::ui,
     * which is the library that exists to hold exactly that arithmetic.
     * What is still worked out here is what a picture of a poker table
     * is: how tall a seat row has to be to be readable, and how wide a
     * card is in glyphs.
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

        /**
         * @brief Work out the picture without drawing it.
         *
         * The picture as a value, so a test can compare it against an
         * expected one rather than watching a renderer being called.
         *
         * Carries no interactions: the table is watched rather than
         * played with, so this frame is handed no pointer.
         *
         * @param canvas The size of the area being drawn into.
         * @param snapshot What to draw.
         * @return The frame, whose commands draw() paints.
         */
        [[nodiscard]] Frame describe(
            Size canvas, const TableSnapshot &snapshot) const;

    private:
        /**
         * @brief What every seat row is laid out against.
         *
         * Gathered once for the whole table, since each of them is a
         * function of the canvas or of the other seats rather than of
         * the seat being drawn.
         */
        struct SeatMetrics
        {
            std::uint32_t rowHeight = 0;
            std::uint32_t barRoom = 0;
            Chips largestStack = 1;
            std::uint32_t scale = 1;
            bool showButton = false;
        };

        void describeHeader(
            Context &ui, const TableSnapshot &snapshot) const;

        void describeBoard(
            Context &ui,
            const TableSnapshot &snapshot,
            std::uint32_t scale) const;

        void describeSeats(
            Context &ui,
            Size canvas,
            const TableSnapshot &snapshot,
            std::uint32_t scale) const;

        void describeSeat(
            Context &ui,
            const SeatSnapshot &seat,
            SeatMetrics metrics) const;

        void describeCards(
            Context &ui,
            std::span<const holdem::Card> cards,
            std::uint32_t scale) const;

        void describeCard(
            Context &ui, holdem::Card card, std::uint32_t scale) const;
    };

} // namespace antwika::poker
