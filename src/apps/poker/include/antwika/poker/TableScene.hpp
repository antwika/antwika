#pragma once

#include <cstdint>
#include <span>

#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/holdem/Card.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Frame.hpp>

#include "antwika/poker/SeatSnapshot.hpp"
#include "antwika/poker/TableSnapshot.hpp"

namespace antwika::poker
{

    using antwika::gfx::Color;
    using antwika::gfx::IRenderer;
    using antwika::gfx::ITexture;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;
    using antwika::ui::Context;
    using antwika::ui::Frame;

    /**
     * @brief One blit of the table's texture atlas.
     *
     * A value rather than a call, for the reason ui::DrawCommand is one:
     * a picture that is a vector of these can be asserted with EXPECT_EQ
     * and no mock at all.
     *
     * It is a second list beside the ui::Frame rather than part of it
     * because antwika::ui draws rectangles and text and nothing else --
     * ui::DrawList has no texture command, and inventing one there would
     * make every ui caller pay for a library that opens no files.
     */
    struct ArtBlit
    {
        /** @brief The region of the atlas to take, in its own pixels. */
        Rect source{};

        /** @brief Where on the canvas it goes; source is scaled to it. */
        Rect destination{};

        /** @brief Multiplied in, so opaque white draws it unchanged. */
        Color tint{};

        bool operator==(const ArtBlit &other) const = default;
    };

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
            const TableSnapshot &snapshot,
            const ITexture *atlas = nullptr) const;

        /**
         * @brief Work out the atlas blits without drawing them.
         *
         * The art layer as a value, for the same reason describe() is
         * one.
         *
         * Painted *before* the frame describe() returns, since the art
         * is the felt, the cards and the furniture, and the text has to
         * read on top of it.
         *
         * @param canvas The size of the area being drawn into.
         * @param snapshot What to draw.
         * @return Every blit, in the order they are painted.
         */
        [[nodiscard]] std::vector<ArtBlit> describeArt(
            Size canvas, const TableSnapshot &snapshot) const;

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

        /**
         * @brief What the art layer is laid out against.
         *
         * Worked out once for the whole canvas, since every one of them
         * is a function of the canvas and the number of seats rather
         * than of the seat being drawn.
         */
        struct ArtMetrics
        {
            std::uint32_t cardWidth = 0;
            std::uint32_t cardHeight = 0;
            std::uint32_t rowHeight = 0;
            std::uint32_t seatTop = 0;
            std::uint32_t boardTop = 0;
        };

        [[nodiscard]] static ArtMetrics artMetricsFor(
            Size canvas, const TableSnapshot &snapshot);

        static void appendFelt(
            std::vector<ArtBlit> &art, Size canvas);

        static void appendBoard(
            std::vector<ArtBlit> &art,
            Size canvas,
            const TableSnapshot &snapshot,
            ArtMetrics metrics);

        static void appendSeats(
            std::vector<ArtBlit> &art,
            Size canvas,
            const TableSnapshot &snapshot,
            ArtMetrics metrics);

        static void appendCard(
            std::vector<ArtBlit> &art,
            holdem::Card card,
            Rect destination,
            bool faceUp);

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
