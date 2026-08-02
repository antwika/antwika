#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Translator.hpp>

#include "antwika/game/SceneSnapshot.hpp"

namespace antwika::game
{

    using antwika::gfx::Color;
    using antwika::gfx::Point;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;
    using antwika::i18n::Translator;

    /**
     * @brief One line of a hover panel, and where it goes.
     */
    struct ReadoutLine
    {
        /** @brief The characters to draw. */
        std::string text;

        /** @brief Top-left corner of the first glyph's cell. */
        Point origin;

        /** @brief What to draw them in. */
        Color colour;

        /**
         * @brief Compare two lines.
         * @param other The line to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const ReadoutLine &other) const
            = default;
    };

    /**
     * @brief A whole hover panel, laid out and ready to paint.
     *
     * A value rather than a sequence of drawing calls, so what the panel
     * says and where it says it can be asserted with EXPECT_EQ.
     * An empty line list is the ordinary answer for a pointer over bare
     * ground, and draws nothing at all.
     */
    struct ReadoutPanel
    {
        /** @brief The backdrop the lines are drawn over. */
        Rect box;

        /** @brief The lines, top to bottom. */
        std::vector<ReadoutLine> lines;

        /**
         * @brief Compare two panels.
         * @param other The panel to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const ReadoutPanel &other) const
            = default;
    };

    /**
     * @brief What the panel is drawn on.
     */
    inline constexpr Color kReadoutBackdrop{
        .red = 16, .green = 18, .blue = 24, .alpha = 225};

    /**
     * @brief What a panel's first line -- the thing's name -- is drawn
     * in.
     */
    inline constexpr Color kReadoutTitle{
        .red = 236, .green = 238, .blue = 236};

    /**
     * @brief The scale a panel's text is measured and drawn at.
     *
     * Public because whoever paints a panel has to draw at the very
     * scale it was laid out against: measuring at one and drawing at
     * another is the second layout this whole value exists to delete.
     * One, which is the toolbar's, so the two read as one application.
     */
    inline constexpr std::uint32_t kReadoutTextScale = 1;

    /**
     * @brief Lay a hover readout out into a panel.
     *
     * **The panel says in words what the bars say as gauges, and it says
     * it about the same resources.** A building is listed for every
     * resource it depends on, which is every one for a house and none
     * for a source that keeps stock nobody drains, and a walker for the
     * one resource its kind carries. One rule rather than two is what
     * keeps a reader from being told two different stories about one
     * building.
     *
     * **Coverage is listed for every kind of building, and only where
     * it is above zero.** Risk is a fact about any building and
     * coverage is what holds it off, so there is no kind the question
     * does not apply to; and a service that has lapsed is not listed at
     * all, because an absent line and a line reading nothing say the
     * same thing.
     *
     * It is pinned near the pointer and then pushed back inside the
     * canvas, so a readout at the far edge of a window is still
     * readable rather than half off it.
     *
     * @param readout What the pointer is over.
     * @param canvas The area it must stay inside.
     * @param translator Words every line; it is a caption a person
     * reads, so it goes through antwika::i18n like the toolbar's.
     * @return The panel, with no lines when there is nothing to say.
     */
    [[nodiscard]] ReadoutPanel readoutPanel(
        const HoverReadout &readout,
        Size canvas,
        const Translator &translator);

} // namespace antwika::game
