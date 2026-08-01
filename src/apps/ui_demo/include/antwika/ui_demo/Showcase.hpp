#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/i18n/MessageId.hpp>

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
     * @brief Get which message names one page.
     *
     * An id rather than the words, so the picker's options are worded
     * by whoever holds the translator and this header holds no language
     * at all.
     * A second table of English names lived here and is gone: it was
     * the picker's list and a log line's both, and two lists of the
     * same nine names drift.
     *
     * @param showcase The page to name.
     * @return Its message id.
     */
    [[nodiscard]] constexpr antwika::i18n::MessageId showcaseNameId(
        const Showcase showcase) noexcept
    {
        constexpr std::array<antwika::i18n::MessageId, kShowcaseCount>
            ids{
                antwika::i18n::MessageId::UiDemoPageLabels,
                antwika::i18n::MessageId::UiDemoPageButtons,
                antwika::i18n::MessageId::UiDemoPageLayout,
                antwika::i18n::MessageId::UiDemoPageTextField,
                antwika::i18n::MessageId::UiDemoPageDropdown,
                antwika::i18n::MessageId::UiDemoPageFocus,
                antwika::i18n::MessageId::UiDemoPageTheme,
                antwika::i18n::MessageId::UiDemoPageRects,
                antwika::i18n::MessageId::UiDemoPageShrink};

        return ids
            [static_cast<std::size_t>(showcase) % kShowcaseCount];
    }

} // namespace antwika::ui_demo
