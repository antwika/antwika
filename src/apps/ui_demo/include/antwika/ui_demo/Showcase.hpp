#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace antwika::ui_demo
{

    /**
     * @brief Which of antwika::ui's elements the demo is showing.
     *
     * One enumerator per page, and the picker lists these names in this
     * order, so a page added here gains its option without a second
     * list that could drift from this one.
     */
    enum class Showcase : std::uint8_t
    {
        /** @brief Text in the theme's colour, in a muted one, and in
         * one of the caller's own. */
        Labels = 0,

        /** @brief Buttons: named, unnamed, forced and sized. */
        Buttons,

        /** @brief Rows, columns and panels nested inside each other. */
        Layout,

        /** @brief A field whose characters this application owns. */
        TextField,

        /** @brief A second list, to show the overlay an open one is. */
        Dropdown,

        /** @brief Tab, Shift+Tab and Enter walking a row of buttons. */
        Focus,

        /** @brief Every colour and metric a ui::Theme carries. */
        Theme,

        /** @brief Where a named widget was laid out, read back. */
        Rects,

        /** @brief A container with less room than its children want. */
        Shrink,
    };

    /**
     * @brief How many pages the showcase has.
     *
     * Derived from the last enumerator rather than written out, so it
     * cannot drift from the enumeration it counts.
     */
    inline constexpr std::size_t kShowcaseCount =
        static_cast<std::size_t>(Showcase::Shrink) + 1;

    /**
     * @brief What each page is called, in Showcase order.
     *
     * A static array rather than something built per frame, because
     * ui::DropdownSpec borrows its options: a temporary vector would be
     * gone before the Context it was handed to laid anything out.
     */
    inline constexpr std::array<std::string_view, kShowcaseCount>
        kShowcaseNames{
            "labels",
            "buttons",
            "layout",
            "text field",
            "dropdown",
            "focus ring",
            "theme",
            "widget rects",
            "shrink"};

    /**
     * @brief Get what one page is called.
     * @param showcase The page to name.
     * @return Its name, exactly as the picker lists it.
     */
    [[nodiscard]] constexpr std::string_view showcaseName(
        Showcase showcase) noexcept
    {
        return kShowcaseNames
            [static_cast<std::size_t>(showcase) % kShowcaseCount];
    }

    /**
     * @brief Get the pages the picker lists.
     * @return A view per name, in the order they are shown.
     */
    [[nodiscard]] inline std::span<const std::string_view>
    showcaseOptions() noexcept
    {
        return std::span<const std::string_view>{kShowcaseNames};
    }

} // namespace antwika::ui_demo
