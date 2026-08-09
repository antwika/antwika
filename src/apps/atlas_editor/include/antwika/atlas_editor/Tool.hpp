#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include <antwika/enums/Enumeration.hpp>

#include "antwika/atlas_editor/MessageId.hpp"

namespace antwika::atlas_editor
{

    enum class Tool : std::uint8_t
    {
        Paint = 0,

        Erase,

        Fill,

        Pick,

        Select,

        Line,

        Ellipse,
    };

    [[nodiscard]] constexpr Tool enumBound(Tool) noexcept
    {
        return Tool::Ellipse;
    }

    inline constexpr std::size_t kToolCount =
        antwika::enums::kCount<Tool>;

    [[nodiscard]] MessageId toolNameId(Tool tool) noexcept;

    [[nodiscard]] std::string_view toolMark(Tool tool) noexcept;

    [[nodiscard]] constexpr bool drawsShape(const Tool tool) noexcept
    {
        return tool == Tool::Line || tool == Tool::Ellipse;
    }

}
