#include "antwika/editor/ui/ToolPanel.hpp"

#include <cstddef>
#include <cstdint>

#include "antwika/editor/ui/WidgetIds.hpp"

namespace antwika::editor
{

    namespace
    {
        [[maybe_unused]] constexpr float kToolStride =
            kToolButtonSide + kToolButtonGap;

        [[maybe_unused]] constexpr std::array kBrushMark{
            Stroke{{0.20F, 0.84F}, {0.46F, 0.48F}},
            Stroke{{0.46F, 0.48F}, {0.58F, 0.62F}},
            Stroke{{0.58F, 0.62F}, {0.20F, 0.84F}},
            Stroke{{0.52F, 0.42F}, {0.82F, 0.16F}},
            Stroke{{0.62F, 0.56F}, {0.86F, 0.30F}}};

        [[maybe_unused]] constexpr std::array kPickerMark{
            Stroke{{0.18F, 0.84F}, {0.56F, 0.46F}},
            Stroke{{0.46F, 0.36F}, {0.72F, 0.62F}},
            Stroke{{0.58F, 0.24F}, {0.84F, 0.50F}},
            Stroke{{0.58F, 0.24F}, {0.46F, 0.36F}},
            Stroke{{0.84F, 0.50F}, {0.72F, 0.62F}}};

        [[maybe_unused]] constexpr std::array kEyeMark{
            Stroke{{0.10F, 0.50F}, {0.30F, 0.32F}},
            Stroke{{0.30F, 0.32F}, {0.70F, 0.32F}},
            Stroke{{0.70F, 0.32F}, {0.90F, 0.50F}},
            Stroke{{0.90F, 0.50F}, {0.70F, 0.68F}},
            Stroke{{0.70F, 0.68F}, {0.30F, 0.68F}},
            Stroke{{0.30F, 0.68F}, {0.10F, 0.50F}},
            Stroke{{0.42F, 0.42F}, {0.58F, 0.42F}},
            Stroke{{0.58F, 0.42F}, {0.58F, 0.58F}},
            Stroke{{0.58F, 0.58F}, {0.42F, 0.58F}},
            Stroke{{0.42F, 0.58F}, {0.42F, 0.42F}}};

        [[maybe_unused]] constexpr std::array kTiesMark{
            Stroke{{0.22F, 0.70F}, {0.78F, 0.30F}},
            Stroke{{0.14F, 0.62F}, {0.30F, 0.62F}},
            Stroke{{0.30F, 0.62F}, {0.30F, 0.78F}},
            Stroke{{0.30F, 0.78F}, {0.14F, 0.78F}},
            Stroke{{0.14F, 0.78F}, {0.14F, 0.62F}},
            Stroke{{0.70F, 0.22F}, {0.86F, 0.22F}},
            Stroke{{0.86F, 0.22F}, {0.86F, 0.38F}},
            Stroke{{0.86F, 0.38F}, {0.70F, 0.38F}},
            Stroke{{0.70F, 0.38F}, {0.70F, 0.22F}}};

        [[maybe_unused]] constexpr std::array kLineMark{
            Stroke{{0.18F, 0.82F}, {0.82F, 0.18F}},
            Stroke{{0.12F, 0.74F}, {0.26F, 0.74F}},
            Stroke{{0.26F, 0.74F}, {0.26F, 0.88F}},
            Stroke{{0.26F, 0.88F}, {0.12F, 0.88F}},
            Stroke{{0.12F, 0.88F}, {0.12F, 0.74F}},
            Stroke{{0.74F, 0.12F}, {0.88F, 0.12F}},
            Stroke{{0.88F, 0.12F}, {0.88F, 0.26F}},
            Stroke{{0.88F, 0.26F}, {0.74F, 0.26F}},
            Stroke{{0.74F, 0.26F}, {0.74F, 0.12F}}};

        [[maybe_unused]] constexpr std::array kFillMark{
            Stroke{{0.22F, 0.46F}, {0.50F, 0.18F}},
            Stroke{{0.50F, 0.18F}, {0.78F, 0.46F}},
            Stroke{{0.78F, 0.46F}, {0.50F, 0.74F}},
            Stroke{{0.50F, 0.74F}, {0.22F, 0.46F}},
            Stroke{{0.84F, 0.58F}, {0.92F, 0.74F}},
            Stroke{{0.92F, 0.74F}, {0.76F, 0.74F}},
            Stroke{{0.76F, 0.74F}, {0.84F, 0.58F}}};

        [[maybe_unused]] constexpr std::array kNormalMark{
            Stroke{{0.22F, 0.28F}, {0.78F, 0.28F}},
            Stroke{{0.78F, 0.28F}, {0.78F, 0.72F}},
            Stroke{{0.78F, 0.72F}, {0.22F, 0.72F}},
            Stroke{{0.22F, 0.72F}, {0.22F, 0.28F}},
            Stroke{{0.22F, 0.46F}, {0.78F, 0.46F}}};

        [[maybe_unused]] constexpr std::array kWaterMark{
            Stroke{{0.14F, 0.40F}, {0.32F, 0.28F}},
            Stroke{{0.32F, 0.28F}, {0.50F, 0.40F}},
            Stroke{{0.50F, 0.40F}, {0.68F, 0.28F}},
            Stroke{{0.68F, 0.28F}, {0.86F, 0.40F}},
            Stroke{{0.14F, 0.66F}, {0.32F, 0.54F}},
            Stroke{{0.32F, 0.54F}, {0.50F, 0.66F}},
            Stroke{{0.50F, 0.66F}, {0.68F, 0.54F}},
            Stroke{{0.68F, 0.54F}, {0.86F, 0.66F}}};

        [[maybe_unused]] constexpr std::array kRampMark{
            Stroke{{0.16F, 0.76F}, {0.84F, 0.24F}},
            Stroke{{0.84F, 0.24F}, {0.84F, 0.76F}},
            Stroke{{0.84F, 0.76F}, {0.16F, 0.76F}}};

        [[maybe_unused]] constexpr std::array kLightMark{
            Stroke{{0.50F, 0.28F}, {0.72F, 0.50F}},
            Stroke{{0.72F, 0.50F}, {0.50F, 0.72F}},
            Stroke{{0.50F, 0.72F}, {0.28F, 0.50F}},
            Stroke{{0.28F, 0.50F}, {0.50F, 0.28F}},
            Stroke{{0.50F, 0.10F}, {0.50F, 0.20F}},
            Stroke{{0.50F, 0.80F}, {0.50F, 0.90F}},
            Stroke{{0.10F, 0.50F}, {0.20F, 0.50F}},
            Stroke{{0.80F, 0.50F}, {0.90F, 0.50F}}};

        [[nodiscard]] std::size_t rankOf(const ToolButton whichButton)
        {
            for (std::size_t rank = 0; rank < kEveryToolButton.size();
                 ++rank)
            {
                if (kEveryToolButton.at(rank) == whichButton)
                {
                    return rank;
                }
            }

            return 0;
        }

        [[nodiscard]] std::size_t rankOf(const voxel::Kind whichKind)
        {
            for (std::size_t rank = 0; rank < voxel::kEveryKind.size();
                 ++rank)
            {
                if (voxel::kEveryKind.at(rank) == whichKind)
                {
                    return rank;
                }
            }

            return 0;
        }

        [[nodiscard]] std::size_t rankOf(const voxel::Facing whichFacing)
        {
            for (std::size_t rank = 0; rank < kMarkedFacings.size();
                 ++rank)
            {
                if (kMarkedFacings.at(rank) == whichFacing)
                {
                    return rank;
                }
            }

            return 0;
        }

        [[nodiscard]] std::size_t rankOf(const voxel::StairHalf whichHalf)
        {
            for (std::size_t rank = 0; rank < kMarkedStairHalves.size();
                 ++rank)
            {
                if (kMarkedStairHalves.at(rank) == whichHalf)
                {
                    return rank;
                }
            }

            return 0;
        }

        [[maybe_unused]] constexpr std::array kLowerMark{
            Stroke{{0.16F, 0.84F}, {0.50F, 0.84F}},
            Stroke{{0.50F, 0.84F}, {0.50F, 0.62F}},
            Stroke{{0.50F, 0.62F}, {0.84F, 0.62F}},
            Stroke{{0.84F, 0.62F}, {0.84F, 0.40F}}};

        [[maybe_unused]] constexpr std::array kUpperMark{
            Stroke{{0.16F, 0.60F}, {0.50F, 0.60F}},
            Stroke{{0.50F, 0.60F}, {0.50F, 0.38F}},
            Stroke{{0.50F, 0.38F}, {0.84F, 0.38F}},
            Stroke{{0.84F, 0.38F}, {0.84F, 0.16F}}};

        [[maybe_unused]] constexpr std::array kWestMark{
            Stroke{{0.78F, 0.50F}, {0.22F, 0.50F}},
            Stroke{{0.22F, 0.50F}, {0.46F, 0.28F}},
            Stroke{{0.22F, 0.50F}, {0.46F, 0.72F}}};

        [[maybe_unused]] constexpr std::array kEastMark{
            Stroke{{0.22F, 0.50F}, {0.78F, 0.50F}},
            Stroke{{0.78F, 0.50F}, {0.54F, 0.28F}},
            Stroke{{0.78F, 0.50F}, {0.54F, 0.72F}}};

        [[maybe_unused]] constexpr std::array kNorthMark{
            Stroke{{0.50F, 0.78F}, {0.50F, 0.22F}},
            Stroke{{0.50F, 0.22F}, {0.28F, 0.46F}},
            Stroke{{0.50F, 0.22F}, {0.72F, 0.46F}}};

        [[maybe_unused]] constexpr std::array kSouthMark{
            Stroke{{0.50F, 0.22F}, {0.50F, 0.78F}},
            Stroke{{0.50F, 0.78F}, {0.28F, 0.54F}},
            Stroke{{0.50F, 0.78F}, {0.72F, 0.54F}}};

        [[nodiscard]] std::size_t rankOf(const Paint whichPaint)
        {
            for (std::size_t rank = 0; rank < kEveryPaint.size();
                 ++rank)
            {
                if (kEveryPaint.at(rank) == whichPaint)
                {
                    return rank;
                }
            }

            return 0;
        }

    }

    namespace
    {
        [[nodiscard]] gfx::Rect iconAt(const std::size_t cell)
        {
            const auto side =
                static_cast<std::uint32_t>(kIconSide);

            return gfx::Rect{
                .originPoint =
                    {.x = static_cast<std::int32_t>(cell)
                          * static_cast<std::int32_t>(side),
                     .y = 0},
                .size = {.width = side, .height = side}};
        }
    }

    gfx::Rect iconOf(const ToolButton whichButton)
    {
        return iconAt(rankOf(whichButton));
    }

    gfx::Rect iconOf(const Paint whichPaint)
    {
        return iconAt(kEveryToolButton.size() + rankOf(whichPaint));
    }

    gfx::Rect iconOf(const voxel::Kind whichKind)
    {
        return iconAt(
            kEveryToolButton.size() + kEveryPaint.size()
            + rankOf(whichKind));
    }

    gfx::Rect iconOf(const voxel::Facing whichFacing)
    {
        return iconAt(
            kEveryToolButton.size() + kEveryPaint.size()
            + voxel::kEveryKind.size() + rankOf(whichFacing));
    }

    gfx::Rect iconOf(const voxel::StairHalf whichHalf)
    {
        return iconAt(
            kEveryToolButton.size() + kEveryPaint.size()
            + voxel::kEveryKind.size() + kMarkedFacings.size()
            + rankOf(whichHalf));
    }

    gfx::Rect getMirrorIcon()
    {
        return iconAt(
            kEveryToolButton.size() + kEveryPaint.size()
            + voxel::kEveryKind.size() + kMarkedFacings.size()
            + kMarkedStairHalves.size());
    }

    widget::WidgetId getKindWidget(const voxel::Kind whichKind)
    {
        return getWidgetAfter(kFirstKindWidget, rankOf(whichKind));
    }

    widget::WidgetId getFacingWidget(const voxel::Facing whichFacing)
    {
        return getWidgetAfter(kFirstFacingWidget, rankOf(whichFacing));
    }

    widget::WidgetId getLevelWidget(const voxel::StairHalf whichHalf)
    {
        return getWidgetAfter(kFirstLevelWidget, rankOf(whichHalf));
    }

    widget::WidgetId getPartWidget(const voxel::StairPart whichPart)
    {
        return whichPart == voxel::StairPart::Side ? kPartSideWidget
                          : kPartFrontWidget;
    }

    widget::WidgetId getCharacterWidget(const std::size_t characterIndex)
    {
        return getWidgetAfter(kFirstCharacterWidget, characterIndex);
    }

    widget::WidgetId getToolWidget(const ToolButton whichButton)
    {
        return getWidgetAfter(kFirstToolWidget, rankOf(whichButton));
    }

    widget::WidgetId getPaintWidget(const Paint whichPaint)
    {
        return getWidgetAfter(kFirstPaintWidget, rankOf(whichPaint));
    }

    std::optional<voxel::Kind> kindFor(
        const input::Key key, const bool ctrlHeld)
    {
        if (ctrlHeld)
        {
            return std::nullopt;
        }

        if (key == input::Key::N)
        {
            return voxel::Kind::Normal;
        }

        if (key == input::Key::R)
        {
            return voxel::Kind::Ramp;
        }

        return std::nullopt;
    }

    std::optional<Paint> paintFor(
        const input::Key key, const bool ctrlHeld)
    {
        if (ctrlHeld)
        {
            return std::nullopt;
        }

        if (key == input::Key::B)
        {
            return Paint::Brush;
        }

        if (key == input::Key::L)
        {
            return Paint::Line;
        }

        if (key == input::Key::F)
        {
            return Paint::Fill;
        }

        if (key == input::Key::M)
        {
            return Paint::Select;
        }

        return std::nullopt;
    }

}
