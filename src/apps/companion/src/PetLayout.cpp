#include "antwika/companion/PetLayout.hpp"

namespace antwika::companion
{

    namespace
    {
        // Where the button sits, in the layout's own units.
        // The grave stands from unit 12 across and unit 11 down.
        // The gauges take the top six rows.
        // The readout takes the last four.
        // So this box is the room left over, and it covers none of them.
        // It does overlap PetScene's kBubble box, deliberately.
        // The two are never up at once, so the room is spent twice.
        // A perished companion is silent: lose() clears the bubble.
        // Pet::requireLivable() refuses a saved one that is not.
        constexpr std::int32_t kButtonX = 1;
        constexpr std::int32_t kButtonY = 7;
        constexpr std::uint32_t kButtonUnitsWide = 12;
        constexpr std::uint32_t kButtonUnitsHigh = 4;
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

        const auto right =
            button->origin.x
            + static_cast<std::int32_t>(button->size.width);
        const auto bottom =
            button->origin.y
            + static_cast<std::int32_t>(button->size.height);

        return at.x >= button->origin.x && at.x < right
               && at.y >= button->origin.y && at.y < bottom;
    }

} // namespace antwika::companion
