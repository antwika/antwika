#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/editor/ui/LayerWidgets.hpp>
#include <antwika/widget/WidgetId.hpp>
#include <antwika/tile/Transitions.hpp>
#include <antwika/decor/Decor.hpp>
#include <antwika/decor/TileAnimation.hpp>
#include <antwika/decor/Variants.hpp>
#include <antwika/map/Layers.hpp>

#include "antwika/editor/plan/PlanBoard.hpp"
#include "antwika/editor/ui/AtlasView.hpp"
#include "antwika/editor/ui/EditorBindings.hpp"
#include "antwika/editor/ui/MapPicker.hpp"
#include "antwika/editor/ui/MenuBar.hpp"
#include "antwika/editor/ui/ToolPanel.hpp"

namespace antwika::editor::widget_catalog
{

    inline constexpr std::array kFixedWidgets{
        widget::kNoWidget,
        kToolPanelWidget,
        kDeriveRulesWidget,
        kStatusBarWidget,
        kLayersPanelWidget,
        kPaletteWidget,
        kRailWidget,
        kPreviewWidget,
        kAddInkWidget,
        kInkOkWidget,
        kInkCancelWidget,
        kInkDeleteWidget,
        kAddLayerWidget,
        kRemoveLayerWidget,
        kMirrorWidget,
        decor::kInkHexWidget,
        decor::kAutoPreviewWidget,
        decor::kPickBaseTilesWidget,
        kPartFrontWidget,
        kPartSideWidget,
        kFirstLayerWidget,
        decor::kFrameAddWidget,
        decor::kTilingPanelWidget,
        decor::kFrequencyWidget,
        decor::kDecorWeightWidget,
        decor::kDecorMoveWidget,
        kQuitConfirmWidget,
        kQuitCancelWidget,
        kQuitAndSaveWidget,
        kPickerNameWidget,
        kPickerConfirmWidget,
        kPickerCancelWidget,
        kPickerOverwriteWidget,
        decor::kRerollPreviewWidget,
        kGlowWidget,
        kAmbientWidget,
        kExitTargetWidget,
        kFigureNameWidget,
        kAddFigureWidget,
        kRemoveFigureWidget,
        kFigureLineWidget,
        kFigureLineAddWidget,
        kFigureLampWidget,
        kFirstMapRowWidget,
        decor::kVariantChoiceWidget,
        decor::kVariantWeightWidget,
        decor::kGoToCanonicalWidget,
        tile::kTransitionAddWidget,
        tile::kRemoveTransitionWidget,
        decor::kToggleAnimationWidget,
        decor::kAddFrameWidget,
        kKeysDoneWidget,
        kKeysResetWidget,
        decor::kSpanAcrossLessWidget,
        decor::kSpanAcrossMoreWidget,
        decor::kSpanDownLessWidget,
        decor::kSpanDownMoreWidget,
        kExitLockedWidget,
        kSheetPanelWidget,
        kDrawPanelWidget,
        kPlanDetailWidget,
        kPlanTitleWidget,
        kPlanBodyWidget,
        kPlanDeleteWidget};

    static_assert(
        widget::allDistinct(kFixedWidgets),
        "two fixed widgets share a number");

    [[nodiscard]] constexpr bool isBlockClearOfFixed(
        const widget::WidgetId baseWidget, const std::size_t width) noexcept
    {
        const auto first = static_cast<std::uint64_t>(baseWidget);

        for (const auto id : kFixedWidgets)
        {
            const auto idValue = static_cast<std::uint64_t>(id);

            if (idValue > first && idValue < first + width)
            {
                return false;
            }
        }

        return true;
    }

    static_assert(
        isBlockClearOfFixed(kFirstLayerWidget, map::kMaxLayers),
        "a fixed widget stands among the layer rows");

    static_assert(
        static_cast<std::uint64_t>(kFirstLayerWidget) + map::kMaxLayers
            <= static_cast<std::uint64_t>(decor::kFrameAddWidget),
        "the layer rows run into the decor frame block");

    static_assert(
        isBlockClearOfFixed(kFirstMapRowWidget, kMaxPicked),
        "a fixed widget stands among the map picker rows");

    static_assert(
        isBlockClearOfFixed(
            getPlanColumnWidget(Column::Todo), kEveryColumn.size()),
        "a fixed widget stands among the plan columns");

    static_assert(
        isBlockClearOfFixed(
            getPlanAddWidget(Column::Todo), kEveryColumn.size()),
        "a fixed widget stands among the plan add buttons");

    static_assert(
        isBlockClearOfFixed(
            getPlanCardWidget(Column::Todo, 0),
            kEveryColumn.size() * kMaxCardsPerColumn),
        "a fixed widget stands among the plan cards");

}
