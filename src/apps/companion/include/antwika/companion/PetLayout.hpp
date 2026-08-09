#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::companion
{

    using antwika::gfx::Point;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;

    inline constexpr std::uint32_t kSceneUnits = 32;

    enum class Prop : std::uint8_t
    {
        Bowl = 0,

        Ball,

        Nest,
    };

    struct SceneLayout final
    {
        std::uint32_t unit = 0;
        Point origin{};

        [[nodiscard]] bool operator==(const SceneLayout &other) const
            = default;
    };

    [[nodiscard]] std::optional<SceneLayout> layoutFor(Size canvas);

    [[nodiscard]] Point point(
        const SceneLayout &layout, std::int32_t x, std::int32_t y);

    [[nodiscard]] Rect box(
        const SceneLayout &layout,
        std::int32_t x,
        std::int32_t y,
        std::uint32_t width,
        std::uint32_t height);

    [[nodiscard]] Rect propBox(const SceneLayout &layout, Prop prop);

    [[nodiscard]] Rect propArtBox(const SceneLayout &layout, Prop prop);

    [[nodiscard]] Rect propLabelBox(const SceneLayout &layout, Prop prop);

    [[nodiscard]] std::optional<Prop> propAt(Size canvas, Point at);

    [[nodiscard]] Rect reviveButtonBox(const SceneLayout &layout);

    [[nodiscard]] std::optional<Rect> reviveButtonRect(Size canvas);

    [[nodiscard]] bool withinReviveButton(Size canvas, Point at);

}
