#include <gtest/gtest.h>

#include <set>
#include <string>

#include <antwika/io/AssetPath.hpp>
#include <antwika/gfx/PngFile.hpp>
#include <antwika/input/Key.hpp>

#include "antwika/editor/ui/ToolPanel.hpp"

namespace
{

    using antwika::voxel::Facing;
    using antwika::editor::facingWidget;
    using antwika::editor::iconOf;
    using antwika::voxel::kEveryKind;
    using antwika::editor::kEveryPaint;
    using antwika::editor::kEveryToolButton;
    using antwika::editor::kIconSide;
    using antwika::editor::kindWidget;
    using antwika::editor::kMarkedFacings;
    using antwika::editor::kMarkedStairHalves;
    using antwika::voxel::Kind;
    using antwika::voxel::StairHalf;
    using antwika::editor::levelWidget;
    using antwika::map::Paint;
    using antwika::editor::paintWidget;
    using antwika::editor::ToolButton;
    using antwika::editor::toolWidget;
    using antwika::input::Key;

    TEST(ToolPanelTest, IconOf_GivesEveryButtonACellOfItsOwn)
    {
        std::set<std::int32_t> lefts;

        const auto check =
            [&lefts](const antwika::gfx::Rect cell)
        {
            EXPECT_EQ(
                cell.size.width,
                static_cast<std::uint32_t>(kIconSide));
            EXPECT_EQ(
                cell.size.height,
                static_cast<std::uint32_t>(kIconSide));
            EXPECT_EQ(cell.originPoint.y, 0);
            EXPECT_TRUE(lefts.insert(cell.originPoint.x).second);
        };

        for (const auto which : kEveryToolButton)
        {
            check(iconOf(which));
        }

        for (const auto which : kEveryPaint)
        {
            check(iconOf(which));
        }

        for (const auto which : kEveryKind)
        {
            check(iconOf(which));
        }

        for (const auto which : kMarkedFacings)
        {
            check(iconOf(which));
        }

        for (const auto which : kMarkedStairHalves)
        {
            check(iconOf(which));
        }

        EXPECT_EQ(lefts.size(), 33U);
    }

    TEST(ToolPanelTest, IconOf_LaysTheCellsInOneRunWithNoGaps)
    {
        std::set<std::int32_t> lefts;

        for (const auto which : kEveryToolButton)
        {
            lefts.insert(iconOf(which).originPoint.x);
        }

        for (const auto which : kEveryPaint)
        {
            lefts.insert(iconOf(which).originPoint.x);
        }

        for (const auto which : kEveryKind)
        {
            lefts.insert(iconOf(which).originPoint.x);
        }

        for (const auto which : kMarkedFacings)
        {
            lefts.insert(iconOf(which).originPoint.x);
        }

        for (const auto which : kMarkedStairHalves)
        {
            lefts.insert(iconOf(which).originPoint.x);
        }

        std::int32_t expectedCount = 0;

        for (const auto left : lefts)
        {
            EXPECT_EQ(left, expectedCount);

            expectedCount += static_cast<std::int32_t>(kIconSide);
        }
    }

    TEST(ToolPanelTest, Widgets_GiveEveryButtonAWidgetOfItsOwn)
    {
        std::set<antwika::ui::WidgetId> seenWidgets;

        for (const auto which : kEveryToolButton)
        {
            EXPECT_TRUE(seenWidgets.insert(toolWidget(which)).second);
        }

        for (const auto which : kEveryPaint)
        {
            EXPECT_TRUE(seenWidgets.insert(paintWidget(which)).second);
        }

        for (const auto which : kEveryKind)
        {
            EXPECT_TRUE(seenWidgets.insert(kindWidget(which)).second);
        }

        for (const auto which : kMarkedFacings)
        {
            EXPECT_TRUE(seenWidgets.insert(facingWidget(which)).second);
        }

        for (const auto which : kMarkedStairHalves)
        {
            EXPECT_TRUE(seenWidgets.insert(levelWidget(which)).second);
        }

        EXPECT_EQ(seenWidgets.size(), 33U);
        EXPECT_FALSE(seenWidgets.contains(antwika::ui::kNoWidget));
        EXPECT_FALSE(
            seenWidgets.contains(antwika::editor::kToolPanelWidget));
    }

    TEST(ToolPanelTest, ToolFor_BindsEachToolToAKey)
    {
        using antwika::editor::toolFor;

        EXPECT_EQ(toolFor(Key::B, false, false), ToolButton::Brush);
        EXPECT_EQ(toolFor(Key::I, false, false), ToolButton::Picker);
        EXPECT_EQ(toolFor(Key::F, true, false), ToolButton::FreeLook);
        EXPECT_EQ(toolFor(Key::L, false, false), ToolButton::Lighting);
        EXPECT_FALSE(toolFor(Key::G, false, false).has_value());
    }

    TEST(ToolPanelTest, KindFor_BindsEachKindToAKey)
    {
        using antwika::editor::kindFor;

        EXPECT_EQ(kindFor(Key::N, false), Kind::Normal);
        EXPECT_EQ(kindFor(Key::R, false), Kind::Ramp);

        EXPECT_FALSE(kindFor(Key::W, false).has_value());
        EXPECT_FALSE(kindFor(Key::N, true).has_value());
    }


    TEST(ToolPanelTest, PaintFor_BindsEachDrawingToolToAKey)
    {
        using antwika::editor::paintFor;

        EXPECT_EQ(paintFor(Key::B, false), Paint::Brush);
        EXPECT_EQ(paintFor(Key::L, false), Paint::Line);
        EXPECT_EQ(paintFor(Key::F, false), Paint::Fill);
        EXPECT_EQ(paintFor(Key::M, false), Paint::Select);
        EXPECT_FALSE(paintFor(Key::G, false).has_value());
    }

    TEST(ToolPanelTest, Icons_HoldACellForEveryButtonDrawn)
    {
        const auto sheet = antwika::gfx::readPngFile(
            antwika::io::assetPath(
                std::string("icons-16.png")),
            "antwika_editor_tests");
        const auto cells =
            kEveryToolButton.size() + kEveryPaint.size()
            + kEveryKind.size() + kMarkedFacings.size()
            + kMarkedStairHalves.size() + 1;

        EXPECT_EQ(
            sheet.size.width,
            cells * static_cast<std::size_t>(kIconSide));
        EXPECT_EQ(
            sheet.size.height,
            static_cast<std::size_t>(kIconSide));
    }

}
