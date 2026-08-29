#pragma once

#include <cstdint>

#include <antwika/gfx/Size.hpp>

namespace antwika::editor
{

    inline constexpr std::uint32_t kMinPanelWidth = 48;

    inline constexpr std::uint32_t kMaxPanelWidth = 4096;

    struct PanelSizes final
    {
        std::uint32_t toolWidth = 0;

        std::uint32_t entityWidth = 0;

        std::uint32_t inspectWidth = 0;

        std::uint32_t railWidth = 0;

        std::uint32_t cardWidth = 0;

        std::uint32_t planFirstWidth = 0;

        std::uint32_t planSecondWidth = 0;

        [[nodiscard]] bool operator==(const PanelSizes &other) const =
            default;
    };

    [[nodiscard]] std::uint32_t getFittedPanelWidth(
        std::uint32_t wish,
        std::uint32_t restingWidth,
        std::uint32_t windowWidth) noexcept;

    [[nodiscard]] float getRailWidthOnCanvas(
        const PanelSizes &panelSizes,
        gfx::Size windowSize,
        gfx::Size canvasSize) noexcept;

}
