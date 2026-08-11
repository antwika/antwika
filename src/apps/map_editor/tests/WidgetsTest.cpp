#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

#include <antwika/ui/WidgetId.hpp>

#include "antwika/map_editor/Widgets.hpp"

namespace widgets = antwika::map_editor::widgets;

using antwika::ui::WidgetId;

TEST(WidgetsTest, TerrainButton_NumbersTheButtonsFromTheBase)
{
    EXPECT_EQ(
        widgets::terrainButton(0),
        static_cast<WidgetId>(widgets::kTerrainBase));
    EXPECT_EQ(
        widgets::terrainButton(widgets::kFreeBrushIndex),
        static_cast<WidgetId>(
            widgets::kTerrainBase + widgets::kFreeBrushIndex));
}

TEST(WidgetsTest, MenuTitle_NumbersTheTitlesFromTheBase)
{
    EXPECT_EQ(
        widgets::menuTitle(0),
        static_cast<WidgetId>(widgets::kMenuBase));
    EXPECT_EQ(
        widgets::menuTitle(3),
        static_cast<WidgetId>(widgets::kMenuBase + 3));
}

TEST(WidgetsTest, MenuIndexOf_FindsTheIndexOfEachTitle)
{
    for (std::size_t index = 0; index < widgets::kMenuCount; ++index)
    {
        EXPECT_EQ(widgets::menuIndexOf(widgets::menuTitle(index)), index);
    }
}

TEST(WidgetsTest, MenuIndexOf_YieldsNothingOutsideTheRange)
{
    EXPECT_FALSE(
        widgets::menuIndexOf(
            static_cast<WidgetId>(widgets::kMenuBase - 1))
            .has_value());
    EXPECT_FALSE(
        widgets::menuIndexOf(
            static_cast<WidgetId>(
                widgets::kMenuBase + widgets::kMenuCount))
            .has_value());
}

TEST(WidgetsTest, DialogRow_NumbersTheRowsFromTheBase)
{
    EXPECT_EQ(
        widgets::dialogRow(0),
        static_cast<WidgetId>(widgets::kDialogRowBase));
    EXPECT_EQ(
        widgets::dialogRow(5),
        static_cast<WidgetId>(widgets::kDialogRowBase + 5));
}

TEST(WidgetsTest, DialogRowIndex_FindsTheIndexInsideTheRowCount)
{
    EXPECT_EQ(widgets::dialogRowIndex(widgets::dialogRow(0), 4), 0U);
    EXPECT_EQ(widgets::dialogRowIndex(widgets::dialogRow(3), 4), 3U);
}

TEST(WidgetsTest, DialogRowIndex_YieldsNothingOutsideTheRowCount)
{
    EXPECT_FALSE(
        widgets::dialogRowIndex(widgets::dialogRow(4), 4).has_value());
    EXPECT_FALSE(
        widgets::dialogRowIndex(
            static_cast<WidgetId>(widgets::kDialogRowBase - 1), 4)
            .has_value());
}

TEST(WidgetsTest, RulesPairButton_LaysTheMatrixOutRowByRow)
{
    EXPECT_EQ(
        widgets::rulesPairButton(0, 0),
        static_cast<WidgetId>(widgets::kRulesPairBase));
    EXPECT_EQ(
        widgets::rulesPairButton(1, 2),
        static_cast<WidgetId>(
            widgets::kRulesPairBase + widgets::kRulesTerrains + 2));
}

TEST(WidgetsTest, RulesPairIndex_FindsTheIndexOfEveryPair)
{
    for (std::size_t row = 0; row < widgets::kRulesTerrains; ++row)
    {
        for (std::size_t column = 0;
             column < widgets::kRulesTerrains;
             ++column)
        {
            EXPECT_EQ(
                widgets::rulesPairIndex(
                    widgets::rulesPairButton(row, column)),
                row * widgets::kRulesTerrains + column);
        }
    }
}

TEST(WidgetsTest, RulesPairIndex_YieldsNothingOutsideTheMatrix)
{
    EXPECT_FALSE(
        widgets::rulesPairIndex(
            static_cast<WidgetId>(widgets::kRulesPairBase - 1))
            .has_value());
    EXPECT_FALSE(
        widgets::rulesPairIndex(
            static_cast<WidgetId>(
                widgets::kRulesPairBase
                + widgets::kRulesTerrains * widgets::kRulesTerrains))
            .has_value());
}

TEST(WidgetsTest, CharacterRow_NumbersTheRowsFromTheBase)
{
    EXPECT_EQ(
        widgets::characterRow(2),
        static_cast<WidgetId>(widgets::kCharRowBase + 2));
}

TEST(WidgetsTest, CharacterRowIndex_FindsTheIndexInsideTheRowCount)
{
    EXPECT_EQ(
        widgets::characterRowIndex(widgets::characterRow(1), 3), 1U);
}

TEST(WidgetsTest, CharacterRowIndex_YieldsNothingOutsideTheRowCount)
{
    EXPECT_FALSE(
        widgets::characterRowIndex(widgets::characterRow(3), 3)
            .has_value());
    EXPECT_FALSE(
        widgets::characterRowIndex(
            static_cast<WidgetId>(widgets::kCharRowBase - 1), 3)
            .has_value());
}

TEST(WidgetsTest, TilesetOption_NumbersTheOptionsFromTheBase)
{
    EXPECT_EQ(
        widgets::tilesetOption(7),
        static_cast<WidgetId>(widgets::kTilesetOptionBase + 7));
}

TEST(WidgetsTest, FrameButton_NumbersTheFramesFromTheBase)
{
    EXPECT_EQ(
        widgets::frameButton(0),
        static_cast<WidgetId>(widgets::kFrameButtonBase));
    EXPECT_EQ(
        widgets::frameButton(widgets::kFrameButtonCount - 1),
        static_cast<WidgetId>(
            widgets::kFrameButtonBase + widgets::kFrameButtonCount - 1));
}

TEST(WidgetsTest, LayerRow_NumbersTheRowsFromTheBase)
{
    EXPECT_EQ(
        widgets::layerRow(3),
        static_cast<WidgetId>(widgets::kLayerRowBase + 3));
}

TEST(WidgetsTest, SocketRow_NumbersTheRowsFromTheBase)
{
    EXPECT_EQ(
        widgets::socketRow(4),
        static_cast<WidgetId>(widgets::kSocketRowBase + 4));
}

TEST(WidgetsTest, BindingPicker_NumbersThePickersFromTheBase)
{
    EXPECT_EQ(
        widgets::bindingPicker(2),
        static_cast<WidgetId>(widgets::kBindingPickerBase + 2));
}

TEST(WidgetsTest, BindingOption_StridesOncePerTerrain)
{
    EXPECT_EQ(
        widgets::bindingOption(0),
        static_cast<WidgetId>(widgets::kBindingOptionBase));
    EXPECT_EQ(
        widgets::bindingOption(2),
        static_cast<WidgetId>(
            widgets::kBindingOptionBase
            + 2 * widgets::kBindingOptionStride));
}

TEST(WidgetsTest, KeysRow_NumbersTheRowsFromTheBase)
{
    EXPECT_EQ(
        widgets::keysRow(5),
        static_cast<WidgetId>(widgets::kKeysRowBase + 5));
}

TEST(WidgetsTest, RangeIndex_FindsTheOffsetInsideTheRange)
{
    EXPECT_EQ(widgets::rangeIndex(static_cast<WidgetId>(12), 10, 4), 2U);
    EXPECT_EQ(widgets::rangeIndex(static_cast<WidgetId>(10), 10, 4), 0U);
}

TEST(WidgetsTest, RangeIndex_YieldsNothingOutsideTheRange)
{
    EXPECT_FALSE(
        widgets::rangeIndex(static_cast<WidgetId>(9), 10, 4)
            .has_value());
    EXPECT_FALSE(
        widgets::rangeIndex(static_cast<WidgetId>(14), 10, 4)
            .has_value());
}

TEST(WidgetsTest, IsField_AcceptsEveryTextField)
{
    for (const auto id : {
             widgets::kFieldId,
             widgets::kFieldTargetMap,
             widgets::kFieldTargetEntry,
             widgets::kFieldTags,
             widgets::kDialogName,
             widgets::kCharName,
             widgets::kPaletteHex,
             widgets::kSocketName,
             widgets::kNewTilesetName})
    {
        EXPECT_TRUE(widgets::isField(id));
    }
}

TEST(WidgetsTest, IsField_RefusesAWidgetThatTakesNoText)
{
    EXPECT_FALSE(widgets::isField(antwika::ui::kNoWidget));
    EXPECT_FALSE(widgets::isField(widgets::kPlace));
    EXPECT_FALSE(widgets::isField(widgets::kDelete));
    EXPECT_FALSE(widgets::isField(widgets::kToolDraw));
}
