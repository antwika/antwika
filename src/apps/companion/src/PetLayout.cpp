#include "antwika/companion/PetLayout.hpp"

#include <array>
#include <cstddef>

namespace antwika::companion
{

    namespace
    {
        // Where the button sits, in the layout's own units.
        // The four gauges take the top eight rows.
        // The grave stands from unit 12 across and unit 12 down.
        // The props take the ground and the readout the last six rows.
        // So this box is the room left over, and it covers none of them.
        // It does overlap PetScene's kBubble box, deliberately.
        // The two are never up at once, so the room is spent twice.
        // A perished companion is silent: perish() clears the bubble.
        // Pet::requireLivable() refuses a saved one that is not.
        constexpr std::int32_t kButtonX = 1;
        constexpr std::int32_t kButtonY = 8;
        constexpr std::uint32_t kButtonUnitsWide = 12;
        constexpr std::uint32_t kButtonUnitsHigh = 4;

        // Where the three props stand, in Prop's own order.
        // All on the ground and all the same size.
        // Spaced so that no two of them share a single unit.
        // A press has to mean exactly one thing.
        // The animal stands between the second and the third of them.
        constexpr std::int32_t kPropY = 22;
        constexpr std::uint32_t kPropUnitsWide = 6;
        constexpr std::uint32_t kPropUnitsHigh = 4;
        constexpr std::array<std::int32_t, 3> kPropX{1, 13, 25};

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
    } // namespace

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
            kPropX[static_cast<std::size_t>(prop)],
            kPropY,
            kPropUnitsWide,
            kPropUnitsHigh);
    }

    std::optional<Prop> propAt(const Size canvas, const Point at)
    {
        const auto layout = layoutFor(canvas);

        if (!layout)
        {
            return std::nullopt;
        }

        // Every prop is asked in order rather than worked backwards.
        // Nothing says the three of them have to stay in a row.
        // So moving one is one number in kPropX and nothing else.
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

} // namespace antwika::companion
