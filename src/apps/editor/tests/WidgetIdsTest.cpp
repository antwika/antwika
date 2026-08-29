#include <gtest/gtest.h>

#include <cstdint>
#include <set>

#include "antwika/editor/ui/AtlasView.hpp"
#include "antwika/editor/ui/LayerWidgets.hpp"
#include "antwika/editor/ui/WidgetIds.hpp"

namespace
{

    using antwika::editor::getFrameWidget;
    using antwika::editor::getMemberWidget;
    using antwika::editor::getSwatchWidget;
    using antwika::editor::kFrameAddWidget;

    TEST(WidgetIdsTest, SwatchWidget_GivesEveryInkASwatchOfItsOwn)
    {
        std::set<antwika::widget::WidgetId> seenWidgets;

        for (std::size_t which = 0; which < antwika::tile::kMaxInks;
             ++which)
        {
            EXPECT_TRUE(
                seenWidgets.insert(getSwatchWidget(which)).second);
        }
    }

    TEST(WidgetIdsTest, FrameWidget_TellsOneFrameFromTheNext)
    {
        EXPECT_NE(getFrameWidget(0), getFrameWidget(1));
        EXPECT_EQ(
            static_cast<std::uint64_t>(getFrameWidget(3))
                - static_cast<std::uint64_t>(getFrameWidget(0)),
            3U);
        EXPECT_NE(getFrameWidget(0), kFrameAddWidget);
    }

    TEST(WidgetIdsTest, MemberWidget_TellsOneMemberFromTheNext)
    {
        EXPECT_NE(getMemberWidget(0), getMemberWidget(1));
        EXPECT_EQ(
            static_cast<std::uint64_t>(getMemberWidget(2))
                - static_cast<std::uint64_t>(getMemberWidget(0)),
            2U);
        EXPECT_NE(getMemberWidget(0), getFrameWidget(0));
    }

    TEST(WidgetIdsTest, WidgetIds_KeepEveryFamilyDomainAndSoloIdApart)
    {
        namespace editor = antwika::editor;
        namespace enums = antwika::enums;

        std::set<antwika::widget::WidgetId> seenWidgets;
        const auto claim =
            [&seenWidgets](const antwika::widget::WidgetId widgetId)
        { EXPECT_TRUE(seenWidgets.insert(widgetId).second); };

        for (const auto soloWidget :
             {editor::kToolPanelWidget,      editor::kWorldPanelWidget,
              editor::kStatusBarWidget,
              editor::kLayersPanelWidget,    editor::kPaletteWidget,
              editor::kRailWidget,           editor::kPreviewWidget,
              editor::kSheetPanelWidget,     editor::kDrawPanelWidget,
              editor::kTilingPanelWidget,    editor::kDeriveRulesWidget,
              editor::kMirrorWidget,         editor::kAddInkWidget,
              editor::kInkOkWidget,          editor::kInkCancelWidget,
              editor::kInkDeleteWidget,      editor::kInkHexWidget,
              editor::kGlowWidget,           editor::kAmbientWidget,
              editor::kAddLayerWidget,       editor::kRemoveLayerWidget,
              editor::kQuitConfirmWidget,    editor::kQuitCancelWidget,
              editor::kQuitAndSaveWidget,    editor::kPickerNameWidget,
              editor::kPickerConfirmWidget,  editor::kPickerCancelWidget,
              editor::kPickerParentFolderWidget, editor::kExitTargetWidget,
              editor::kCharacterNameWidget,
              editor::kAddCharacterWidget,      editor::kRemoveCharacterWidget,
              editor::kCharacterLineWidget,     editor::kCharacterLineAddWidget,
              editor::kCharacterLampWidget,     editor::kKeysDoneWidget,
              editor::kKeysResetWidget,      editor::kPartFrontWidget,
              editor::kPartSideWidget,       editor::kBoundaryToggleWidget,
              editor::kForbiddenToggleWidget, editor::kAutoPreviewWidget,
              editor::kRerollPreviewWidget,  editor::kPickBaseTilesWidget,
              editor::kFrameAddWidget,       editor::kFrequencyWidget,
              editor::kDecorWeightWidget,    editor::kDecorMoveWidget,
              editor::kSpanAcrossLessWidget, editor::kSpanAcrossMoreWidget,
              editor::kSpanDownLessWidget,   editor::kSpanDownMoreWidget,
              editor::kVariantChoiceWidget,  editor::kVariantWeightWidget,
              editor::kGoToCanonicalWidget,  editor::kTransitionAddWidget,
              editor::kRemoveTransitionWidget,
              editor::kToggleAnimationWidget, editor::kAddFrameWidget,
              editor::kPlanDetailWidget,     editor::kPlanTitleWidget,
              editor::kPlanBodyWidget,       editor::kPlanDeleteWidget,
              editor::kComponentAddOpenWidget,
              editor::kMarkerRemoveWidget,
              editor::kGizmoClearWidget,
              editor::kEntityRemoveWidget,
              editor::kEntityListPanelWidget,
              editor::kEntityListScrollWidget,
              editor::kToolPanelEdgeWidget,
              editor::kEntityListEdgeWidget,
              editor::kDrawColumnWidget,
              editor::kDrawColumnEdgeWidget,
              editor::kRailEdgeWidget,
              editor::kPlanFirstEdgeWidget,
              editor::kPlanSecondEdgeWidget,
              editor::kPlanDetailEdgeWidget})
        {
            claim(soloWidget);
        }

        for (const auto menu : enums::kAll<editor::Menu>)
        {
            claim(editor::getMenuWidget(menu));

            for (std::uint64_t item = 0;
                 item < editor::kMaxItemsPerMenu;
                 ++item)
            {
                claim(
                    editor::getWidgetAfter(
                        editor::getFirstItemWidget(menu), item));
            }
        }

        for (const auto view : enums::kAll<editor::View>)
        {
            claim(editor::getTabWidget(view));
        }

        for (const auto toolButton : editor::kEveryToolButton)
        {
            claim(editor::getToolWidget(toolButton));
        }

        for (const auto paint : editor::kEveryPaint)
        {
            claim(editor::getPaintWidget(paint));
        }

        for (const auto kind : enums::kAll<antwika::voxel::Kind>)
        {
            claim(editor::getKindWidget(kind));
        }

        for (const auto facing : editor::kMarkedFacings)
        {
            claim(editor::getFacingWidget(facing));
        }

        for (const auto stairHalf : editor::kMarkedStairHalves)
        {
            claim(editor::getLevelWidget(stairHalf));
        }

        for (std::size_t ink = 0; ink < antwika::tile::kMaxInks; ++ink)
        {
            claim(getSwatchWidget(ink));
        }

        for (std::size_t layer = 0; layer < antwika::map::kMaxLayers;
             ++layer)
        {
            claim(editor::getLayerWidget(layer));
        }

        for (std::size_t frame = 0;
             frame < antwika::decor::kMaxDecorFrames;
             ++frame)
        {
            claim(getFrameWidget(frame));
            claim(editor::getFlipFrameWidget(frame));
        }

        for (std::size_t row = 0; row < antwika::tile::kMaxTransitions;
             ++row)
        {
            claim(editor::getTransitionRowWidget(row));
        }

        for (std::size_t member = 0;
             member < static_cast<std::size_t>(
                          antwika::decor::kMaxDecorSpan)
                          * antwika::decor::kMaxDecorSpan;
             ++member)
        {
            claim(getMemberWidget(member));
        }

        for (std::size_t row = 0; row < editor::kMaxPickedRows; ++row)
        {
            claim(editor::getMapRowWidget(row));
        }

        for (std::size_t character = 0;
             character < editor::kMaxCharacterWidgets;
             ++character)
        {
            claim(editor::getCharacterWidget(character));
        }

        for (std::size_t row = 0; row < editor::kActionCount; ++row)
        {
            claim(editor::getKeyRowWidget(row));
        }

        for (const auto column : editor::kEveryColumn)
        {
            claim(editor::getPlanAddWidget(column));
            claim(editor::getPlanColumnWidget(column));

            for (std::size_t card = 0;
                 card < editor::kMaxCardsPerColumn;
                 ++card)
            {
                claim(editor::getPlanCardWidget(column, card));
            }
        }

        for (std::size_t slot = 0; slot < editor::kMaxComponentSlots;
             ++slot)
        {
            claim(editor::getComponentHeadWidget(slot));
            claim(editor::getComponentDropWidget(slot));
            claim(editor::getComponentAddWidget(slot));

            for (std::size_t field = 0;
                 field < editor::kMaxComponentFields;
                 ++field)
            {
                claim(editor::getComponentFieldWidget(slot, field));
            }
        }

        claim(editor::kComponentScrollWidget);

        for (std::uint64_t axis = 0; axis < editor::kMarkerAxisCount;
             ++axis)
        {
            claim(editor::getEntityFieldWidget(axis));
        }

        for (std::uint64_t axis = 0; axis < editor::kMarkerAxisCount;
             ++axis)
        {
            claim(editor::getMarkerFieldWidget(axis));
        }

        for (std::size_t row = 0; row < editor::kMaxEntityRows; ++row)
        {
            claim(editor::getEntityRowWidget(row));
        }
    }

    TEST(WidgetIdsTest, PlanEdgeWidget_NamesABarForEveryColumnButTheLast)
    {
        namespace editor = antwika::editor;

        EXPECT_EQ(
            editor::getPlanEdgeWidget(editor::Column::Todo),
            editor::kPlanFirstEdgeWidget);
        EXPECT_EQ(
            editor::getPlanEdgeWidget(editor::Column::Doing),
            editor::kPlanSecondEdgeWidget);
        EXPECT_EQ(
            editor::getPlanEdgeWidget(editor::Column::Done),
            antwika::widget::kNoWidget);
    }

}
