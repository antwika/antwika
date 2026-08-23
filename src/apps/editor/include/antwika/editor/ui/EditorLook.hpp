#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/ISurfaceRenderer.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Theme.hpp>

namespace antwika::editor
{

    inline constexpr gfx::Color kEditorBackgroundColor{
        .red = 72, .green = 74, .blue = 88};

    inline constexpr gfx::Color kPlayBackgroundColor{
        .red = 8, .green = 8, .blue = 12};

    inline constexpr gfx::Color kTextColor{
        .red = 232, .green = 232, .blue = 224};

    inline constexpr gfx::Color kWhiteColor{
        .red = 255, .green = 255, .blue = 255, .alpha = 255};

    inline constexpr gfx::Color kDisabledTintColor{
        .red = 255, .green = 255, .blue = 255, .alpha = 70};

    inline constexpr gfx::Color kSelectionAccentColor{
        .red = 255, .green = 196, .blue = 92, .alpha = 255};

    inline constexpr gfx::Color kRuleLineColor{
        .red = 255, .green = 236, .blue = 140, .alpha = 255};

    inline constexpr gfx::Color kHealthBarEmptyColor{
        .red = 24, .green = 24, .blue = 32, .alpha = 200};

    inline constexpr gfx::Color kFoodBarColor{
        .red = 140, .green = 200, .blue = 96, .alpha = 255};

    inline constexpr gfx::Color kWaterBarColor{
        .red = 96, .green = 176, .blue = 232, .alpha = 255};

    inline constexpr gfx::Color kRuleLineCrossLevelColor{
        .red = 128, .green = 220, .blue = 255, .alpha = 255};

    inline constexpr gfx::Color kLevelGridLineColor{
        .red = 96, .green = 99, .blue = 118, .alpha = 255};

    inline constexpr gfx::Color kPlacementPreviewColor{
        .red = 255, .green = 232, .blue = 160, .alpha = 255};

    inline constexpr gfx::Color kForbiddenMarkerColor{
        .red = 208, .green = 84, .blue = 84, .alpha = 255};

    inline constexpr gfx::Color kBoundaryMarkerColor{
        .red = 128, .green = 220, .blue = 255, .alpha = 255};

    inline constexpr float kBoundaryBandThickness = 2.0F;

    inline constexpr float kEdgeToggleLabelInset = 3.0F;

    inline constexpr gfx::Color kCornerSeamLineColor{
        .red = 255, .green = 170, .blue = 110, .alpha = 255};

    inline constexpr gfx::Color kCornerFilledMarkerColor{
        .red = 132, .green = 226, .blue = 160, .alpha = 255};

    inline constexpr gfx::Color kCornerEmptyMarkerColor{
        .red = 236, .green = 140, .blue = 224, .alpha = 255};

    inline constexpr gfx::Color kEmptyCellMarkerColor{
        .red = 132, .green = 136, .blue = 158, .alpha = 255};

    inline constexpr float kEmptyMarkerArmFraction = 0.22F;

    inline constexpr gfx::Color kGridLineColor{
        .red = 44, .green = 44, .blue = 56, .alpha = 255};

    inline constexpr gfx::Color kPanelColor{
        .red = 56, .green = 58, .blue = 70, .alpha = 255};

    inline constexpr gfx::Color kTitleBarColor{
        .red = 38, .green = 40, .blue = 50, .alpha = 255};

    inline constexpr float kCursorArmLength = 3.0F;

    inline constexpr gfx::Color kCursorColor{
        .red = 255, .green = 226, .blue = 64, .alpha = 255};

    inline constexpr float kCursorThickness = 1.0F;

    inline constexpr gfx::Color kInteriorRuleLineColor{
        .red = 132, .green = 226, .blue = 160, .alpha = 255};

    inline constexpr gfx::Color kBoundaryRuleLineColor{
        .red = 236, .green = 140, .blue = 224, .alpha = 255};

    inline constexpr gfx::Color kVariantLinkLineColor{
        .red = 240, .green = 200, .blue = 96, .alpha = 255};

    inline constexpr std::uint32_t kCaptionCharTicks = 2;

    inline constexpr std::uint32_t kCaptionHoldTicks = 240;

    inline constexpr float kBloomStrength = 0.85F;

    inline constexpr std::uint32_t kUiScale = 2;

    inline constexpr std::size_t kMaxMenuLines = 16;

    inline constexpr std::uint32_t kPanelGap = 6;

    inline constexpr std::uint32_t kPanelPadding = 2 * kUiScale;

    inline constexpr std::uint32_t kSwatchWidth = 12;

    inline constexpr std::uint32_t kPreviewScale = 3;

    inline constexpr std::uint32_t kPickerWidth = 150;

    [[nodiscard]] inline ui::Theme getGameTheme()
    {
        return ui::Theme{
            .panelColor = kPanelColor,
            .textColor = kTextColor,
            .buttonIdleColor = kGridLineColor,
            .buttonHoveredColor = kPanelColor,
            .buttonPressedColor = kSelectionAccentColor,
            .buttonTextColor = kTextColor,
            .focusRingColor = kSelectionAccentColor,
            .textScale = kUiScale,
            .padding = 4 * kUiScale,
            .gap = 2 * kUiScale,
            .buttonPadding = 2 * kUiScale,
            .checkboxSize = 5 * kUiScale,
            .checkboxInset = kUiScale,
            .focusRingThickness = 0};
    } // GCOVR_EXCL_LINE

    [[nodiscard]] inline ui::Theme getMenuTheme()
    {
        auto look = getGameTheme();

        look.face = ui::TextFace::Small;
        look.textScale = (3 * kUiScale) / 2;
        look.buttonPadding = 3 * kUiScale;
        look.gap = 3 * kUiScale;
        look.checkboxSize = 5 * kUiScale;
        look.checkboxInset = kUiScale;

        return look;
    } // GCOVR_EXCL_LINE

    [[nodiscard]] inline std::string getCapitalized(
        const std::string_view text)
    {
        std::string capitalText{text};

        if (!capitalText.empty() && capitalText.front() >= 'a'
            && capitalText.front() <= 'z')
        {
            capitalText.front() = static_cast<char>(
                (capitalText.front() - 'a') + 'A');
        }

        return capitalText;
    } // GCOVR_EXCL_LINE

    inline void panelTitle(
        ui::Context &context, const std::string_view name)
    {
        const auto bar = context.row(
            ui::ContainerSpec{
                .widthSizing = ui::kGrowSizing,
                .backgroundColor = kTitleBarColor,
                .padding = kPanelPadding});

        context.label(name, kTextColor);
    }

    inline void drawOutline(
        gfx::ISurfaceRenderer &renderer,
        const gfx::RectF whereRect,
        const gfx::Color lineColor)
    {
        const auto left = whereRect.originPoint.x;
        const auto top = whereRect.originPoint.y;
        const auto right = left + whereRect.size.width;
        const auto foot = top + whereRect.size.height;

        for (const auto &[fromPoint, toPoint] :
             {std::pair{gfx::PointF{left, top}, gfx::PointF{right, top}},
              std::pair{gfx::PointF{left, foot}, gfx::PointF{right, foot}},
              std::pair{gfx::PointF{left, top}, gfx::PointF{left, foot}},
              std::pair{gfx::PointF{right, top}, gfx::PointF{right, foot}}})
        {
            renderer.drawLine(fromPoint, toPoint, lineColor);
        }
    }

    inline constexpr float kPaneMargin = 6.0F;

    inline constexpr float kTopBarHeight = 24.0F;

    inline constexpr float kInspectColumnWidth = 104.0F;

    inline constexpr float kToolPanelWidth = 68.0F;

    inline constexpr float kBottomBarHeight = 12.0F;

    inline constexpr float kRightPanelWidth = 64.0F;

}
