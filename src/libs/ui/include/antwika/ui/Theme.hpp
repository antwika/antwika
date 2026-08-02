#pragma once

#include <cstdint>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::ui
{

    using antwika::gfx::Color;
    using antwika::gfx::Size;

    /**
     * @brief The colours and pixel metrics every widget draws from.
     *
     * Every field is defaulted, so Theme{} is the default theme and
     * changing one colour is one designated initialiser.
     *
     * Holds what a widget picks without being told.
     * Sizing and alignment are not here, because those are decided per
     * call, and nothing here is remembered between frames.
     */
    struct Theme
    {
        /**
         * @brief Behind a panel.
         */
        Color panel{.red = 28, .green = 30, .blue = 38};

        /**
         * @brief Ordinary text.
         */
        Color text{.red = 232, .green = 236, .blue = 232};

        /**
         * @brief Text that should read as secondary.
         */
        Color muted{.red = 120, .green = 140, .blue = 128};

        /**
         * @brief Behind a button at rest.
         */
        Color buttonIdle{.red = 48, .green = 52, .blue = 64};

        /**
         * @brief Behind a button the caller reports as hovered.
         */
        Color buttonHovered{.red = 68, .green = 74, .blue = 92};

        /**
         * @brief Behind a button the caller reports as pressed.
         */
        Color buttonPressed{.red = 32, .green = 36, .blue = 44};

        /**
         * @brief A button's own label.
         */
        Color buttonText{.red = 240, .green = 240, .blue = 240};

        /**
         * @brief Behind a text field that has not got focus.
         */
        Color field{.red = 20, .green = 22, .blue = 28};

        /**
         * @brief Behind the text field the typing belongs to.
         */
        Color fieldFocused{.red = 14, .green = 16, .blue = 20};

        /**
         * @brief The bar drawn where a focused field's caret sits.
         */
        Color caret{.red = 232, .green = 236, .blue = 232};

        /**
         * @brief Behind the characters a text area has selected.
         *
         * Behind rather than instead of: the text keeps its own colour,
         * so a selection reads as the same characters on a different
         * ground rather than as different characters.
         */
        Color selection{.red = 44, .green = 72, .blue = 116};

        /**
         * @brief The ground behind a text area's highlighted spans.
         *
         * A caller marks spans through TextAreaSpec::highlights -- a
         * live-coding editor lights the notes that are sounding -- and
         * they read as the same characters on different ground, the
         * argument selection makes.  The selection wins where the two
         * overlap, since it is what the next keystroke acts on.
         */
        Color highlight{.red = 38, .green = 84, .blue = 52};

        /**
         * @brief The channel a text area's scrollbar runs in.
         */
        Color scrollTrack{.red = 30, .green = 33, .blue = 42};

        /**
         * @brief The part of that bar standing for what is showing.
         */
        Color scrollThumb{.red = 78, .green = 86, .blue = 106};

        /**
         * @brief The border drawn around the focused widget.
         *
         * Yellow, because focus has to read as focus against every
         * button colour above it rather than as one more shade of them.
         */
        Color focusRing{.red = 244, .green = 208, .blue = 63};

        /**
         * @brief Pixels per glyph pixel for every label.
         */
        std::uint32_t textScale = 1;

        /**
         * @brief Inset on every side of a panel.
         */
        std::uint32_t padding = 4;

        /**
         * @brief Space between one child of a container and the next.
         */
        std::uint32_t gap = 4;

        /**
         * @brief Inset on every side of a button's label.
         */
        std::uint32_t buttonPadding = 6;

        /**
         * @brief How thick the focused widget's border is.
         *
         * Zero draws no border at all, which is how an application that
         * wants keyboard focus without a ring asks for it.
         */
        std::uint32_t focusRingThickness = 2;

        /**
         * @brief How wide a text area's scrollbar is.
         *
         * Taken out of the room the text has, so an area that asks for
         * one is that many pixels narrower to write in.
         */
        std::uint32_t scrollbarWidth = 8;
    };

    /**
     * @brief Pick a glyph scale to suit a canvas.
     *
     * Text below this is hard to read and above it wastes the window.
     * The same heuristic apps/poker's TableScene kept to itself.
     *
     * @param canvas The area being drawn into.
     * @return At least one, whatever the canvas.
     */
    [[nodiscard]] std::uint32_t scaleForCanvas(Size canvas) noexcept;

    /**
     * @brief Multiply every pixel metric in a theme.
     *
     * Colours are carried over untouched, since a colour does not get
     * bigger on a bigger screen.
     *
     * @param base The theme to scale.
     * @param scale Pixels per theme pixel; zero leaves no metrics at all.
     * @return The scaled theme.
     */
    [[nodiscard]] Theme scaledTheme(
        Theme base, std::uint32_t scale) noexcept;

} // namespace antwika::ui
