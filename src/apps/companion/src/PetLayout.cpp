#include "antwika/companion/PetLayout.hpp"

#include <array>
#include <cstddef>

namespace antwika::companion
{

    namespace
    {
        constexpr std::int32_t kButtonX = 1;
        constexpr std::int32_t kButtonY = 8;
        constexpr std::uint32_t kButtonUnitsWide = 12;
        constexpr std::uint32_t kButtonUnitsHigh = 4;

        constexpr std::int32_t kPropY = 22;
        constexpr std::uint32_t kPropUnitsWide = 6;
        constexpr std::uint32_t kPropUnitsHigh = 4;
        constexpr std::array<std::int32_t, 3> kPropX{1, 13, 25};

        constexpr std::uint32_t kPropLabelUnitsHigh = 1;
        constexpr std::uint32_t kPropArtUnitsHigh =
            kPropUnitsHigh - kPropLabelUnitsHigh;

        [[nodiscard]] std::int32_t propColumn(const Prop prop)
        {
            return kPropX[static_cast<std::size_t>(prop)];
        }

        [[nodiscard]] bool within(const Rect &area, const Point at)
        {
            const auto right =
                area.origin.x + static_cast<std::int32_t>(area.size.width);
            const auto bottom =
                area.origin.y
                + static_cast<std::int32_t>(area.size.height);

            return at.x >= area.origin.x && at.x < right
                   && at.y >= area.origin.y && at.y < bottom;
        }
    }

    std::optional<SceneLayout> layoutFor(const Size canvas)
    {
        const auto byWidth = canvas.width / kSceneUnits;
        const auto byHeight = canvas.height / kSceneUnits;
        const auto unit = byWidth < byHeight ? byWidth : byHeight;

        if (unit == 0)
        {
            return std::nullopt;
        }

        const auto used = unit * kSceneUnits;

        return SceneLayout{
            .unit = unit,
            .origin = {
                .x = static_cast<std::int32_t>((canvas.width - used) / 2),
                .y = static_cast<std::int32_t>(
                    (canvas.height - used) / 2)}};
    }

    Point point(
        const SceneLayout &layout,
        const std::int32_t x,
        const std::int32_t y)
    {
        const auto unit = static_cast<std::int32_t>(layout.unit);

        return Point{
            .x = layout.origin.x + x * unit,
            .y = layout.origin.y + y * unit};
    }

    Rect box(
        const SceneLayout &layout,
        const std::int32_t x,
        const std::int32_t y,
        const std::uint32_t width,
        const std::uint32_t height)
    {
        return Rect{
            .origin = point(layout, x, y),
            .size = {
                .width = width * layout.unit,
                .height = height * layout.unit}};
    }

    Rect propBox(const SceneLayout &layout, const Prop prop)
    {
        return box(
            layout,
            propColumn(prop),
            kPropY,
            kPropUnitsWide,
            kPropUnitsHigh);
    }

    Rect propArtBox(const SceneLayout &layout, const Prop prop)
    {
        return box(
            layout,
            propColumn(prop),
            kPropY,
            kPropUnitsWide,
            kPropArtUnitsHigh);
    }

    Rect propLabelBox(const SceneLayout &layout, const Prop prop)
    {
        return box(
            layout,
            propColumn(prop),
            kPropY + static_cast<std::int32_t>(kPropArtUnitsHigh),
            kPropUnitsWide,
            kPropLabelUnitsHigh);
    }

    std::optional<Prop> propAt(const Size canvas, const Point at)
    {
        const auto layout = layoutFor(canvas);

        if (!layout)
        {
            return std::nullopt;
        }

        for (std::size_t index = 0; index < kPropX.size(); ++index)
        {
            const auto prop = static_cast<Prop>(index);

            if (within(propBox(*layout, prop), at))
            {
                return prop;
            }
        }

        return std::nullopt;
    }

    Rect reviveButtonBox(const SceneLayout &layout)
    {
        return box(
            layout, kButtonX, kButtonY, kButtonUnitsWide,
            kButtonUnitsHigh);
    }

    std::optional<Rect> reviveButtonRect(const Size canvas)
    {
        const auto layout = layoutFor(canvas);

        if (!layout)
        {
            return std::nullopt;
        }

        return reviveButtonBox(*layout);
    }

    bool withinReviveButton(const Size canvas, const Point at)
    {
        const auto button = reviveButtonRect(canvas);

        if (!button)
        {
            return false;
        }

        return within(*button, at);
    }

}
