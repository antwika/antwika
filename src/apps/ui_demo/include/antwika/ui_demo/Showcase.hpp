#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/enums/Enumeration.hpp>

#include "antwika/ui_demo/MessageId.hpp"

namespace antwika::ui_demo
{

    enum class Showcase : std::uint8_t
    {
        Labels = 0,

        Buttons,

        Layout,

        TextField,

        Dropdown,

        Focus,

        Theme,

        Rects,

        Shrink,

        TextArea,
    };

    [[nodiscard]] constexpr Showcase enumBound(Showcase) noexcept
    {
        return Showcase::TextArea;
    }

    inline constexpr std::size_t kShowcaseCount =
        antwika::enums::kCount<Showcase>;

    [[nodiscard]] constexpr MessageId showcaseNameId(
        const Showcase showcase) noexcept
    {
        constexpr std::array<MessageId, kShowcaseCount>
            ids{
                MessageId::PageLabels,
                MessageId::PageButtons,
                MessageId::PageLayout,
                MessageId::PageTextField,
                MessageId::PageDropdown,
                MessageId::PageFocus,
                MessageId::PageTheme,
                MessageId::PageRects,
                MessageId::PageShrink,
                MessageId::PageTextArea};

        return ids
            [static_cast<std::size_t>(showcase) % kShowcaseCount];
    }

}
