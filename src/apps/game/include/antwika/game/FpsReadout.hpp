#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>

namespace antwika::game
{

    using antwika::gfx::Size;
    using antwika::ui::DrawList;

    inline constexpr std::string_view kNoRateReadout{"fps --"};

    [[nodiscard]] DrawList describeFps(
        Size canvas, std::optional<std::uint32_t> framesPerSecond);

}
