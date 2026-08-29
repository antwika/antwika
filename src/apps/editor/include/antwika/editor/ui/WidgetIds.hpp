#pragma once

#include <cstddef>
#include <cstdint>

#include <antwika/decor/Decor.hpp>
#include <antwika/enums/Enumeration.hpp>
#include <antwika/map/Layers.hpp>
#include <antwika/tile/TilePaint.hpp>
#include <antwika/tile/Transitions.hpp>
#include <antwika/voxel/VoxelMaterial.hpp>
#include <antwika/widget/WidgetId.hpp>

#include "antwika/editor/Preferences.hpp"
#include "antwika/editor/plan/PlanBoard.hpp"
#include "antwika/editor/ui/EditorBindings.hpp"
#include "antwika/editor/ui/MapPicker.hpp"
#include "antwika/editor/ui/MenuBar.hpp"
#include "antwika/editor/ui/ToolPanel.hpp"

namespace antwika::editor
{

    [[nodiscard]] constexpr widget::WidgetId getWidgetAfter(
        const widget::WidgetId widgetId,
        const std::uint64_t width = 1) noexcept
    {
        return widget::WidgetId{
            static_cast<std::uint64_t>(widgetId) + width};
    }

    inline constexpr widget::WidgetId kToolPanelWidget =
        getWidgetAfter(widget::kNoWidget);

    inline constexpr widget::WidgetId kWorldPanelWidget =
        getWidgetAfter(kToolPanelWidget);

    inline constexpr widget::WidgetId kStatusBarWidget =
        getWidgetAfter(kWorldPanelWidget);

    inline constexpr widget::WidgetId kLayersPanelWidget =
        getWidgetAfter(kStatusBarWidget);

    inline constexpr widget::WidgetId kPaletteWidget =
        getWidgetAfter(kLayersPanelWidget);

    inline constexpr widget::WidgetId kRailWidget =
        getWidgetAfter(kPaletteWidget);

    inline constexpr widget::WidgetId kPreviewWidget =
        getWidgetAfter(kRailWidget);

    inline constexpr widget::WidgetId kSheetPanelWidget =
        getWidgetAfter(kPreviewWidget);

    inline constexpr widget::WidgetId kDrawPanelWidget =
        getWidgetAfter(kSheetPanelWidget);

    inline constexpr widget::WidgetId kTilingPanelWidget =
        getWidgetAfter(kDrawPanelWidget);

    inline constexpr widget::WidgetId kDeriveRulesWidget =
        getWidgetAfter(kTilingPanelWidget);

    inline constexpr widget::WidgetId kMirrorWidget =
        getWidgetAfter(kDeriveRulesWidget);

    inline constexpr widget::WidgetId kAddInkWidget =
        getWidgetAfter(kMirrorWidget);

    inline constexpr widget::WidgetId kInkOkWidget =
        getWidgetAfter(kAddInkWidget);

    inline constexpr widget::WidgetId kInkCancelWidget =
        getWidgetAfter(kInkOkWidget);

    inline constexpr widget::WidgetId kInkDeleteWidget =
        getWidgetAfter(kInkCancelWidget);

    inline constexpr widget::WidgetId kInkHexWidget =
        getWidgetAfter(kInkDeleteWidget);

    inline constexpr widget::WidgetId kGlowWidget =
        getWidgetAfter(kInkHexWidget);

    inline constexpr widget::WidgetId kAmbientWidget =
        getWidgetAfter(kGlowWidget);

    inline constexpr widget::WidgetId kAddLayerWidget =
        getWidgetAfter(kAmbientWidget);

    inline constexpr widget::WidgetId kRemoveLayerWidget =
        getWidgetAfter(kAddLayerWidget);

    inline constexpr widget::WidgetId kQuitConfirmWidget =
        getWidgetAfter(kRemoveLayerWidget);

    inline constexpr widget::WidgetId kQuitCancelWidget =
        getWidgetAfter(kQuitConfirmWidget);

    inline constexpr widget::WidgetId kQuitAndSaveWidget =
        getWidgetAfter(kQuitCancelWidget);

    inline constexpr widget::WidgetId kPickerNameWidget =
        getWidgetAfter(kQuitAndSaveWidget);

    inline constexpr widget::WidgetId kPickerConfirmWidget =
        getWidgetAfter(kPickerNameWidget);

    inline constexpr widget::WidgetId kPickerCancelWidget =
        getWidgetAfter(kPickerConfirmWidget);

    inline constexpr widget::WidgetId kPickerParentFolderWidget =
        getWidgetAfter(kPickerCancelWidget);

    inline constexpr widget::WidgetId kExitTargetWidget =
        getWidgetAfter(kPickerParentFolderWidget);

    inline constexpr widget::WidgetId kCharacterNameWidget =
        getWidgetAfter(kExitTargetWidget);

    inline constexpr widget::WidgetId kAddCharacterWidget =
        getWidgetAfter(kCharacterNameWidget);

    inline constexpr widget::WidgetId kRemoveCharacterWidget =
        getWidgetAfter(kAddCharacterWidget);

    inline constexpr widget::WidgetId kCharacterLineWidget =
        getWidgetAfter(kRemoveCharacterWidget);

    inline constexpr widget::WidgetId kCharacterLineAddWidget =
        getWidgetAfter(kCharacterLineWidget);

    inline constexpr widget::WidgetId kCharacterLampWidget =
        getWidgetAfter(kCharacterLineAddWidget);

    inline constexpr widget::WidgetId kKeysDoneWidget =
        getWidgetAfter(kCharacterLampWidget);

    inline constexpr widget::WidgetId kKeysResetWidget =
        getWidgetAfter(kKeysDoneWidget);

    inline constexpr widget::WidgetId kPartFrontWidget =
        getWidgetAfter(kKeysResetWidget);

    inline constexpr widget::WidgetId kPartSideWidget =
        getWidgetAfter(kPartFrontWidget);

    inline constexpr widget::WidgetId kBoundaryToggleWidget =
        getWidgetAfter(kPartSideWidget);

    inline constexpr widget::WidgetId kForbiddenToggleWidget =
        getWidgetAfter(kBoundaryToggleWidget);

    inline constexpr widget::WidgetId kAutoPreviewWidget =
        getWidgetAfter(kForbiddenToggleWidget);

    inline constexpr widget::WidgetId kRerollPreviewWidget =
        getWidgetAfter(kAutoPreviewWidget);

    inline constexpr widget::WidgetId kPickBaseTilesWidget =
        getWidgetAfter(kRerollPreviewWidget);

    inline constexpr widget::WidgetId kFrameAddWidget =
        getWidgetAfter(kPickBaseTilesWidget);

    inline constexpr widget::WidgetId kFrequencyWidget =
        getWidgetAfter(kFrameAddWidget);

    inline constexpr widget::WidgetId kDecorWeightWidget =
        getWidgetAfter(kFrequencyWidget);

    inline constexpr widget::WidgetId kDecorMoveWidget =
        getWidgetAfter(kDecorWeightWidget);

    inline constexpr widget::WidgetId kSpanAcrossLessWidget =
        getWidgetAfter(kDecorMoveWidget);

    inline constexpr widget::WidgetId kSpanAcrossMoreWidget =
        getWidgetAfter(kSpanAcrossLessWidget);

    inline constexpr widget::WidgetId kSpanDownLessWidget =
        getWidgetAfter(kSpanAcrossMoreWidget);

    inline constexpr widget::WidgetId kSpanDownMoreWidget =
        getWidgetAfter(kSpanDownLessWidget);

    inline constexpr widget::WidgetId kVariantChoiceWidget =
        getWidgetAfter(kSpanDownMoreWidget);

    inline constexpr widget::WidgetId kVariantWeightWidget =
        getWidgetAfter(kVariantChoiceWidget);

    inline constexpr widget::WidgetId kGoToCanonicalWidget =
        getWidgetAfter(kVariantWeightWidget);

    inline constexpr widget::WidgetId kTransitionAddWidget =
        getWidgetAfter(kGoToCanonicalWidget);

    inline constexpr widget::WidgetId kRemoveTransitionWidget =
        getWidgetAfter(kTransitionAddWidget);

    inline constexpr widget::WidgetId kToggleAnimationWidget =
        getWidgetAfter(kRemoveTransitionWidget);

    inline constexpr widget::WidgetId kAddFrameWidget =
        getWidgetAfter(kToggleAnimationWidget);

    inline constexpr widget::WidgetId kPlanDetailWidget =
        getWidgetAfter(kAddFrameWidget);

    inline constexpr widget::WidgetId kPlanTitleWidget =
        getWidgetAfter(kPlanDetailWidget);

    inline constexpr widget::WidgetId kPlanBodyWidget =
        getWidgetAfter(kPlanTitleWidget);

    inline constexpr widget::WidgetId kPlanDeleteWidget =
        getWidgetAfter(kPlanBodyWidget);

    inline constexpr std::uint64_t kMaxItemsPerMenu = 16;

    inline constexpr std::uint64_t kMaxCharacterWidgets = 64;

    inline constexpr std::uint64_t kMaxComponentSlots = 16;

    inline constexpr std::uint64_t kMaxComponentFields = 4;

    inline constexpr widget::WidgetId kFirstMenuWidget =
        getWidgetAfter(kPlanDeleteWidget);

    inline constexpr widget::WidgetId kFirstMenuItemWidget =
        getWidgetAfter(kFirstMenuWidget, enums::kCount<Menu>);

    inline constexpr widget::WidgetId kFirstTabWidget =
        getWidgetAfter(
            kFirstMenuItemWidget,
            enums::kCount<Menu> * kMaxItemsPerMenu);

    inline constexpr widget::WidgetId kFirstToolWidget =
        getWidgetAfter(kFirstTabWidget, enums::kCount<View>);

    inline constexpr widget::WidgetId kFirstPaintWidget =
        getWidgetAfter(kFirstToolWidget, enums::kCount<ToolButton>);

    inline constexpr widget::WidgetId kFirstKindWidget =
        getWidgetAfter(kFirstPaintWidget, enums::kCount<Paint>);

    inline constexpr widget::WidgetId kFirstFacingWidget =
        getWidgetAfter(kFirstKindWidget, enums::kCount<voxel::Kind>);

    inline constexpr widget::WidgetId kFirstLevelWidget =
        getWidgetAfter(kFirstFacingWidget, kMarkedFacings.size());

    inline constexpr widget::WidgetId kFirstSwatchWidget =
        getWidgetAfter(kFirstLevelWidget, kMarkedStairHalves.size());

    inline constexpr widget::WidgetId kFirstLayerWidget =
        getWidgetAfter(kFirstSwatchWidget, tile::kMaxInks);

    inline constexpr widget::WidgetId kFirstFrameWidget =
        getWidgetAfter(kFirstLayerWidget, map::kMaxLayers);

    inline constexpr widget::WidgetId kFirstFlipFrameWidget =
        getWidgetAfter(kFirstFrameWidget, decor::kMaxDecorFrames);

    inline constexpr widget::WidgetId kFirstTransitionRowWidget =
        getWidgetAfter(kFirstFlipFrameWidget, decor::kMaxDecorFrames);

    inline constexpr widget::WidgetId kFirstMemberWidget =
        getWidgetAfter(kFirstTransitionRowWidget, tile::kMaxTransitions);

    inline constexpr widget::WidgetId kFirstMapRowWidget =
        getWidgetAfter(
            kFirstMemberWidget,
            static_cast<std::uint64_t>(decor::kMaxDecorSpan)
                * decor::kMaxDecorSpan);

    inline constexpr widget::WidgetId kFirstCharacterWidget =
        getWidgetAfter(kFirstMapRowWidget, kMaxPickedRows);

    inline constexpr widget::WidgetId kFirstKeyRowWidget =
        getWidgetAfter(kFirstCharacterWidget, kMaxCharacterWidgets);

    inline constexpr widget::WidgetId kFirstPlanAddWidget =
        getWidgetAfter(kFirstKeyRowWidget, kActionCount);

    inline constexpr widget::WidgetId kFirstPlanColumnWidget =
        getWidgetAfter(kFirstPlanAddWidget, kEveryColumn.size());

    inline constexpr widget::WidgetId kFirstPlanCardWidget =
        getWidgetAfter(kFirstPlanColumnWidget, kEveryColumn.size());

    inline constexpr widget::WidgetId kComponentAddOpenWidget =
        getWidgetAfter(
            kFirstPlanCardWidget,
            kEveryColumn.size() * kMaxCardsPerColumn);

    inline constexpr widget::WidgetId kFirstComponentHeadWidget =
        getWidgetAfter(kComponentAddOpenWidget);

    inline constexpr widget::WidgetId kFirstComponentDropWidget =
        getWidgetAfter(kFirstComponentHeadWidget, kMaxComponentSlots);

    inline constexpr widget::WidgetId kFirstComponentAddWidget =
        getWidgetAfter(kFirstComponentDropWidget, kMaxComponentSlots);

    inline constexpr widget::WidgetId kFirstComponentFieldWidget =
        getWidgetAfter(kFirstComponentAddWidget, kMaxComponentSlots);

    inline constexpr std::uint64_t kMarkerAxisCount = 3;

    inline constexpr widget::WidgetId kFirstMarkerFieldWidget =
        getWidgetAfter(
            kFirstComponentFieldWidget,
            kMaxComponentSlots * kMaxComponentFields);

    [[nodiscard]] constexpr widget::WidgetId getSwatchWidget(
        const std::size_t which) noexcept
    {
        return getWidgetAfter(kFirstSwatchWidget, which);
    }

    [[nodiscard]] constexpr widget::WidgetId getFrameWidget(
        const std::size_t frame) noexcept
    {
        return getWidgetAfter(kFirstFrameWidget, frame);
    }

    [[nodiscard]] constexpr widget::WidgetId getFlipFrameWidget(
        const std::size_t frame) noexcept
    {
        return getWidgetAfter(kFirstFlipFrameWidget, frame);
    }

    [[nodiscard]] constexpr widget::WidgetId getTransitionRowWidget(
        const std::size_t rowIndex) noexcept
    {
        return getWidgetAfter(kFirstTransitionRowWidget, rowIndex);
    }

    [[nodiscard]] constexpr widget::WidgetId getMemberWidget(
        const std::size_t member) noexcept
    {
        return getWidgetAfter(kFirstMemberWidget, member);
    }

    [[nodiscard]] constexpr widget::WidgetId getPlanColumnWidget(
        const Column whichColumn) noexcept
    {
        return getWidgetAfter(
            kFirstPlanColumnWidget,
            static_cast<std::uint64_t>(whichColumn));
    }

    [[nodiscard]] constexpr widget::WidgetId getPlanAddWidget(
        const Column whichColumn) noexcept
    {
        return getWidgetAfter(
            kFirstPlanAddWidget,
            static_cast<std::uint64_t>(whichColumn));
    }

    [[nodiscard]] constexpr widget::WidgetId getPlanCardWidget(
        const Column whichColumn, const std::size_t cardIndex) noexcept
    {
        return getWidgetAfter(
            kFirstPlanCardWidget,
            (static_cast<std::uint64_t>(whichColumn)
             * kMaxCardsPerColumn)
                + cardIndex);
    }

    [[nodiscard]] constexpr widget::WidgetId getComponentHeadWidget(
        const std::size_t slot) noexcept
    {
        return getWidgetAfter(kFirstComponentHeadWidget, slot);
    }

    [[nodiscard]] constexpr widget::WidgetId getComponentDropWidget(
        const std::size_t slot) noexcept
    {
        return getWidgetAfter(kFirstComponentDropWidget, slot);
    }

    [[nodiscard]] constexpr widget::WidgetId getComponentAddWidget(
        const std::size_t rowIndex) noexcept
    {
        return getWidgetAfter(kFirstComponentAddWidget, rowIndex);
    }

    [[nodiscard]] constexpr widget::WidgetId getComponentFieldWidget(
        const std::size_t slot, const std::size_t field) noexcept
    {
        return getWidgetAfter(
            kFirstComponentFieldWidget,
            (slot * kMaxComponentFields) + field);
    }

    inline constexpr widget::WidgetId kComponentScrollWidget =
        getWidgetAfter(kFirstMarkerFieldWidget, kMarkerAxisCount);

    inline constexpr widget::WidgetId kMarkerRemoveWidget =
        getWidgetAfter(kComponentScrollWidget);

    inline constexpr widget::WidgetId kGizmoClearWidget =
        getWidgetAfter(kMarkerRemoveWidget);

    inline constexpr widget::WidgetId kEntityRemoveWidget =
        getWidgetAfter(kGizmoClearWidget);

    inline constexpr widget::WidgetId kFirstEntityFieldWidget =
        getWidgetAfter(kEntityRemoveWidget);

    inline constexpr std::size_t kMaxEntityRows = 128;

    inline constexpr widget::WidgetId kEntityListPanelWidget =
        getWidgetAfter(kFirstEntityFieldWidget, kMarkerAxisCount);

    inline constexpr widget::WidgetId kEntityListScrollWidget =
        getWidgetAfter(kEntityListPanelWidget);

    inline constexpr widget::WidgetId kFirstEntityRowWidget =
        getWidgetAfter(kEntityListScrollWidget);

    [[nodiscard]] constexpr widget::WidgetId getEntityRowWidget(
        const std::size_t row) noexcept
    {
        return getWidgetAfter(kFirstEntityRowWidget, row);
    }

    inline constexpr widget::WidgetId kToolPanelEdgeWidget =
        getWidgetAfter(kFirstEntityRowWidget, kMaxEntityRows);

    inline constexpr widget::WidgetId kEntityListEdgeWidget =
        getWidgetAfter(kToolPanelEdgeWidget);

    inline constexpr widget::WidgetId kDrawColumnWidget =
        getWidgetAfter(kEntityListEdgeWidget);

    inline constexpr widget::WidgetId kDrawColumnEdgeWidget =
        getWidgetAfter(kDrawColumnWidget);

    inline constexpr widget::WidgetId kRailEdgeWidget =
        getWidgetAfter(kDrawColumnEdgeWidget);

    inline constexpr widget::WidgetId kPlanFirstEdgeWidget =
        getWidgetAfter(kRailEdgeWidget);

    inline constexpr widget::WidgetId kPlanSecondEdgeWidget =
        getWidgetAfter(kPlanFirstEdgeWidget);

    inline constexpr widget::WidgetId kPlanDetailEdgeWidget =
        getWidgetAfter(kPlanSecondEdgeWidget);

    [[nodiscard]] constexpr widget::WidgetId getPlanEdgeWidget(
        const Column whichColumn) noexcept
    {
        return whichColumn == Column::Todo  ? kPlanFirstEdgeWidget
             : whichColumn == Column::Doing ? kPlanSecondEdgeWidget
                                            : widget::kNoWidget;
    }

    [[nodiscard]] constexpr widget::WidgetId getMarkerFieldWidget(
        const std::size_t axis) noexcept
    {
        return getWidgetAfter(kFirstMarkerFieldWidget, axis);
    }

    [[nodiscard]] constexpr widget::WidgetId getEntityFieldWidget(
        const std::size_t axis) noexcept
    {
        return getWidgetAfter(kFirstEntityFieldWidget, axis);
    }

}
