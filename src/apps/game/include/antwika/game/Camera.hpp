#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Point.hpp>

namespace antwika::game
{

    using antwika::gfx::Point;

    inline constexpr std::array<std::uint32_t, 5> kZoomHalfWidths{
        4, 8, 16, 32, 64};

    inline constexpr std::size_t kDefaultZoomLevel = 3;

    class Camera final
    {
    public:
        constexpr explicit Camera(
            Point pan = {}, std::size_t zoomLevel = kDefaultZoomLevel)
            : panOffset(pan), zoom(clampZoom(zoomLevel))
        {
        }

        [[nodiscard]] constexpr Point pan() const noexcept
        {
            return panOffset;
        }

        [[nodiscard]] constexpr std::size_t zoomLevel() const noexcept
        {
            return zoom;
        }

        [[nodiscard]] constexpr std::uint32_t halfWidth() const noexcept
        {
            return kZoomHalfWidths[zoom];
        }

        [[nodiscard]] constexpr std::uint32_t halfHeight() const noexcept
        {
            return halfWidth() / 2;
        }

        constexpr void panBy(std::int32_t dx, std::int32_t dy) noexcept
        {
            panOffset.x += dx;
            panOffset.y += dy;
        }

        constexpr void setPan(Point pan) noexcept
        {
            panOffset = pan;
        }

        constexpr void zoomIn() noexcept
        {
            if (zoom + 1 < kZoomHalfWidths.size())
            {
                ++zoom;
            }
        }

        constexpr void zoomOut() noexcept
        {
            if (zoom > 0)
            {
                --zoom;
            }
        }

        [[nodiscard]] bool operator==(const Camera &other) const = default;

    private:
        [[nodiscard]] static constexpr std::size_t clampZoom(
            std::size_t level) noexcept
        {
            return level < kZoomHalfWidths.size()
                       ? level
                       : kZoomHalfWidths.size() - 1;
        }

        Point panOffset;
        std::size_t zoom;
    };

}
