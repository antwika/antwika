#include <gtest/gtest.h>

#include <cstdint>
#include <string>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/TextLayout.hpp>

#include "TestTranslator.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/ReadoutPanel.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/ResourceBar.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/Walker.hpp"

using antwika::game::tests::kTranslator;

using antwika::game::BuildingKind;
using antwika::game::BuildingSprite;
using antwika::game::Cell;
using antwika::game::HoverReadout;
using antwika::game::kCoverageFull;
using antwika::game::kReadoutTextScale;
using antwika::game::kReadoutTitle;
using antwika::game::kStockCapacity;
using antwika::game::kWalkerLoad;
using antwika::game::readoutPanel;
using antwika::game::Resource;
using antwika::game::resourceColour;
using antwika::game::Service;
using antwika::game::serviceColour;
using antwika::game::WalkerKind;
using antwika::game::WalkerSprite;
using antwika::gfx::Point;
using antwika::gfx::Size;

namespace
{
    constexpr Size kCanvas{.width = 640, .height = 480};

    [[nodiscard]] HoverReadout over(BuildingSprite building)
    {
        return HoverReadout{
            .anchor = Point{.x = 100, .y = 90}, .building = building};
    }

    [[nodiscard]] HoverReadout over(WalkerSprite walker)
    {
        return HoverReadout{
            .anchor = Point{.x = 100, .y = 90}, .walker = walker};
    }
} // namespace

TEST(ReadoutPanelTest, Panel_SaysNothingWithNothingUnderThePointer)
{
    const auto panel = readoutPanel(HoverReadout{}, kCanvas, kTranslator);

    EXPECT_TRUE(panel.lines.empty());
    EXPECT_EQ(panel.box.size, Size{});
}

TEST(ReadoutPanelTest, Panel_NamesABuildingAndListsWhatItDependsOn)
{
    const auto panel = readoutPanel(
        over(
            BuildingSprite{
                .at = Cell{.x = 1, .y = 1},
                .kind = BuildingKind::House,
                .stock = {12, 34, 56}}),
        kCanvas,
        kTranslator);

    ASSERT_EQ(panel.lines.size(), 4U);
    EXPECT_EQ(panel.lines[0].text, "house");
    EXPECT_EQ(panel.lines[1].text, "food 12/100");
    EXPECT_EQ(panel.lines[2].text, "clay 34/100");
    EXPECT_EQ(panel.lines[3].text, "pottery 56/100");
    EXPECT_EQ(kStockCapacity, 100);
}

// The panel says in words what the bars say as gauges.
// One rule, so a reader is never told two stories about one building.
TEST(ReadoutPanelTest, Panel_NamesASourceAndListsNoStockItNeverSpends)
{
    const auto panel = readoutPanel(
        over(
            BuildingSprite{
                .at = Cell{},
                .kind = BuildingKind::Farm,
                .stock = {90, 90, 90}}),
        kCanvas,
        kTranslator);

    ASSERT_EQ(panel.lines.size(), 1U);
    EXPECT_EQ(panel.lines[0].text, "farm");
}

TEST(ReadoutPanelTest, Panel_NamesAWalkerAndWhatItIsCarrying)
{
    const auto panel = readoutPanel(
        over(
            WalkerSprite{
                .at = Cell{},
                .kind = WalkerKind::MarketSeller,
                .carried = 60}),
        kCanvas,
        kTranslator);

    ASSERT_EQ(panel.lines.size(), 2U);
    EXPECT_EQ(panel.lines[0].text, "market seller");
    EXPECT_EQ(panel.lines[1].text, "food 60/100");
    EXPECT_EQ(kWalkerLoad, 100);
}

TEST(ReadoutPanelTest, Panel_NamesAWalkerThatCarriesNothingAndStops)
{
    const auto panel = readoutPanel(
        over(
            WalkerSprite{.at = Cell{}, .kind = WalkerKind::WaterCarrier}),
        kCanvas,
        kTranslator);

    ASSERT_EQ(panel.lines.size(), 1U);
    EXPECT_EQ(panel.lines[0].text, "water carrier");
}

// A line and the bar beside it count the same thing.
// So they are the same colour, out of the one table.
TEST(ReadoutPanelTest, Panel_ColoursALineAsTheBarThatCountsTheSameThing)
{
    const auto panel = readoutPanel(
        over(
            BuildingSprite{
                .at = Cell{},
                .kind = BuildingKind::House,
                .stock = {1, 2, 3}}),
        kCanvas,
        kTranslator);

    ASSERT_EQ(panel.lines.size(), 4U);
    EXPECT_EQ(panel.lines[0].colour, kReadoutTitle);
    EXPECT_EQ(panel.lines[1].colour, resourceColour(Resource::Food));
    EXPECT_EQ(panel.lines[2].colour, resourceColour(Resource::Clay));
    EXPECT_EQ(panel.lines[3].colour, resourceColour(Resource::Pottery));
}

TEST(ReadoutPanelTest, Panel_HoldsEveryLineInsideItsOwnBox)
{
    const auto panel = readoutPanel(
        over(
            BuildingSprite{
                .at = Cell{},
                .kind = BuildingKind::House,
                .stock = {100, 7, 0}}),
        kCanvas,
        kTranslator);

    ASSERT_FALSE(panel.lines.empty());

    const auto right = panel.box.origin.x
        + static_cast<std::int32_t>(panel.box.size.width);
    const auto bottom = panel.box.origin.y
        + static_cast<std::int32_t>(panel.box.size.height);

    for (const auto &line : panel.lines)
    {
        const auto measured =
            antwika::gfx::textSize(line.text, kReadoutTextScale);

        EXPECT_GE(line.origin.x, panel.box.origin.x);
        EXPECT_GE(line.origin.y, panel.box.origin.y);
        EXPECT_LE(
            line.origin.x + static_cast<std::int32_t>(measured.width),
            right);
        EXPECT_LE(
            line.origin.y + static_cast<std::int32_t>(measured.height),
            bottom);
    }
}

// A readout at the far corner is still readable rather than half off.
TEST(ReadoutPanelTest, Panel_IsPushedBackInsideTheCanvasAtAFarCorner)
{
    HoverReadout readout = over(
        BuildingSprite{
            .at = Cell{},
            .kind = BuildingKind::House,
            .stock = {50, 50}});
    readout.anchor = Point{
        .x = static_cast<std::int32_t>(kCanvas.width) - 2,
        .y = static_cast<std::int32_t>(kCanvas.height) - 2};

    const auto panel = readoutPanel(readout, kCanvas, kTranslator);

    EXPECT_LE(
        panel.box.origin.x
            + static_cast<std::int32_t>(panel.box.size.width),
        static_cast<std::int32_t>(kCanvas.width));
    EXPECT_LE(
        panel.box.origin.y
            + static_cast<std::int32_t>(panel.box.size.height),
        static_cast<std::int32_t>(kCanvas.height));
}

// A canvas too small to hold one is the underflow trap blog/012 found.
TEST(ReadoutPanelTest, Panel_StaysOnACanvasSmallerThanItself)
{
    const auto panel = readoutPanel(
        over(
            BuildingSprite{
                .at = Cell{},
                .kind = BuildingKind::House,
                .stock = {50, 50, 50}}),
        Size{.width = 4, .height = 4},
        kTranslator);

    EXPECT_EQ(panel.box.origin, (Point{.x = 0, .y = 0}));
    EXPECT_FALSE(panel.lines.empty());
}

TEST(ReadoutPanelTest, EqualityComparesEveryField)
{
    const auto base = readoutPanel(
        over(
            BuildingSprite{
                .at = Cell{},
                .kind = BuildingKind::House,
                .stock = {1, 2, 3}}),
        kCanvas,
        kTranslator);

    EXPECT_EQ(base, base);

    auto moved = base;
    moved.box.origin.x += 1;
    EXPECT_NE(base, moved);

    auto shorter = base;
    shorter.lines.pop_back();
    EXPECT_NE(base, shorter);

    auto reworded = base;
    reworded.lines[0].text = "hovel";
    EXPECT_NE(base, reworded);

    auto shifted = base;
    shifted.lines[0].origin.y += 1;
    EXPECT_NE(base, shifted);

    auto recoloured = base;
    recoloured.lines[0].colour = resourceColour(Resource::Clay);
    EXPECT_NE(base, recoloured);
}

// Coverage is listed for every kind of building, not only a house.
// Risk is a fact about any building, and coverage is what holds it off.
TEST(ReadoutPanelTest, Panel_ListsEveryServiceThatStillReachesABuilding)
{
    const auto panel = readoutPanel(
        over(
            BuildingSprite{
                .at = Cell{},
                .kind = BuildingKind::Well,
                .coverage = {kCoverageFull, 0, kCoverageFull / 2, 0}}),
        kCanvas,
        kTranslator);

    ASSERT_EQ(panel.lines.size(), 3U);
    EXPECT_EQ(panel.lines[0].text, "well");
    EXPECT_EQ(panel.lines[1].text, "water 100%");
    EXPECT_EQ(panel.lines[2].text, "safety 50%");
}

// A service that has lapsed is not listed at all.
// An absent line and a line reading nothing say one thing.
TEST(ReadoutPanelTest, Panel_ListsNoServiceThatHasLapsed)
{
    const auto panel = readoutPanel(
        over(
            BuildingSprite{
                .at = Cell{},
                .kind = BuildingKind::House,
                .stock = {1, 2, 3}}),
        kCanvas,
        kTranslator);

    ASSERT_EQ(panel.lines.size(), 4U);
    EXPECT_EQ(panel.lines[0].text, "house");
}

TEST(ReadoutPanelTest, Panel_ColoursACoverageLineOutOfTheServiceTable)
{
    const auto panel = readoutPanel(
        over(
            BuildingSprite{
                .at = Cell{},
                .kind = BuildingKind::Doctor,
                .coverage = {0, kCoverageFull, 0, 0}}),
        kCanvas,
        kTranslator);

    ASSERT_EQ(panel.lines.size(), 2U);
    EXPECT_EQ(panel.lines[1].colour, serviceColour(Service::Health));
}
