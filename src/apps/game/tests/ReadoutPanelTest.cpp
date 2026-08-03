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
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/HousingQuery.hpp"
#include "antwika/game/ReadoutPanel.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/ResourceColour.hpp"
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

    ASSERT_EQ(panel.lines.size(), 14U);
    EXPECT_EQ(panel.lines[0].text, "house");
    EXPECT_EQ(panel.lines[1].text, "level: tent");
    EXPECT_EQ(panel.lines[2].text, "people 0/5");
    EXPECT_EQ(panel.lines[3].text, "unemployed 0/0");
    EXPECT_EQ(panel.lines[4].text, "resources");
    EXPECT_EQ(panel.lines[5].text, "  food 12/100");
    EXPECT_EQ(panel.lines[6].text, "  clay 34/100");
    EXPECT_EQ(panel.lines[7].text, "  pottery 56/100");
    EXPECT_EQ(panel.lines[8].text, "  water 0/100");
    EXPECT_EQ(panel.lines[9].text, "  medicine 0/100");
    EXPECT_EQ(panel.lines[10].text, "risk");
    EXPECT_EQ(panel.lines[11].text, "  fire 0%");
    EXPECT_EQ(panel.lines[12].text, "  collapse 0%");
    EXPECT_EQ(panel.lines[13].text, "  disease 0%");
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

    ASSERT_EQ(panel.lines.size(), 8U);
    EXPECT_EQ(panel.lines[0].text, "farm");
    EXPECT_EQ(panel.lines[1].text, "staff 0/4");

    // The medicine is on every building, even at nothing.
    // Water is a house's alone: only a household drinks it.
    // The stock a source keeps and nobody drains stays unlisted.
    EXPECT_EQ(panel.lines[2].text, "resources");
    EXPECT_EQ(panel.lines[3].text, "  medicine 0/100");

    // The risk section is on every building, even at nothing.
    // A watched district's danger is a fact somebody wants at zero.
    EXPECT_EQ(panel.lines[4].text, "risk");
    EXPECT_EQ(panel.lines[5].text, "  fire 0%");
    EXPECT_EQ(panel.lines[6].text, "  collapse 0%");
    EXPECT_EQ(panel.lines[7].text, "  disease 0%");
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

    ASSERT_EQ(panel.lines.size(), 14U);
    EXPECT_EQ(panel.lines[0].colour, kReadoutTitle);
    EXPECT_EQ(panel.lines[1].colour, kReadoutTitle);
    EXPECT_EQ(panel.lines[2].colour, kReadoutTitle);
    EXPECT_EQ(panel.lines[3].colour, kReadoutTitle);
    EXPECT_EQ(panel.lines[4].colour, kReadoutTitle);
    EXPECT_EQ(panel.lines[5].colour, resourceColour(Resource::Food));
    EXPECT_EQ(panel.lines[6].colour, resourceColour(Resource::Clay));
    EXPECT_EQ(
        panel.lines[7].colour, resourceColour(Resource::Pottery));
    EXPECT_EQ(panel.lines[8].colour, serviceColour(Service::Water));
    EXPECT_EQ(panel.lines[9].colour, serviceColour(Service::Health));

    // A risk line wears its own ink, named once beside the table.
    EXPECT_EQ(panel.lines[10].colour, kReadoutTitle);
    EXPECT_EQ(panel.lines[11].colour, antwika::game::kFireRiskInk);
    EXPECT_EQ(
        panel.lines[12].colour, antwika::game::kCollapseRiskInk);
    EXPECT_EQ(
        panel.lines[13].colour, antwika::game::kDiseaseRiskInk);
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

// The medicine is listed for every kind of building.
// Water is a house's alone: only a household drinks it.
TEST(ReadoutPanelTest, Panel_ListsTheMedicineOnAnyBuilding)
{
    const auto panel = readoutPanel(
        over(
            BuildingSprite{
                .at = Cell{},
                .kind = BuildingKind::Well,
                .coverage = {kCoverageFull, kCoverageFull / 2}}),
        kCanvas,
        kTranslator);

    ASSERT_EQ(panel.lines.size(), 8U);
    EXPECT_EQ(panel.lines[0].text, "well");
    EXPECT_EQ(panel.lines[1].text, "staff 0/1");
    EXPECT_EQ(panel.lines[2].text, "resources");
    EXPECT_EQ(panel.lines[3].text, "  medicine 50/100");
    EXPECT_EQ(panel.lines[4].text, "risk");
}

// The tier is named rather than numbered.
// "level: 2" would be a number a reader has to look up.
TEST(ReadoutPanelTest, Panel_NamesTheTierAHouseIsOn)
{
    const auto panel = readoutPanel(
        over(
            BuildingSprite{
                .at = Cell{},
                .kind = BuildingKind::House,
                .level = antwika::game::HousingLevel::Cottage}),
        kCanvas,
        kTranslator);

    ASSERT_GE(panel.lines.size(), 2U);
    EXPECT_EQ(panel.lines[1].text, "level: cottage");
}

// Water and medicine are said even at nothing.
// Zero is exactly the state a watcher wants warned of.
TEST(ReadoutPanelTest, Panel_SaysWaterAndMedicineEvenAtNothing)
{
    const auto panel = readoutPanel(
        over(
            BuildingSprite{
                .at = Cell{},
                .kind = BuildingKind::House,
                .stock = {1, 2, 3}}),
        kCanvas,
        kTranslator);

    ASSERT_EQ(panel.lines.size(), 14U);
    EXPECT_EQ(panel.lines[0].text, "house");
    EXPECT_EQ(panel.lines[8].text, "  water 0/100");
    EXPECT_EQ(panel.lines[9].text, "  medicine 0/100");
}

// How full a house is, against what its tier has room for.
// A bare count is a number a reader cannot do anything with.
TEST(ReadoutPanelTest, Panel_SaysHowFullAHouseIs)
{
    const auto panel = readoutPanel(
        over(
            BuildingSprite{
                .at = Cell{},
                .kind = BuildingKind::House,
                .level = antwika::game::HousingLevel::Hovel,
                .population = 9}),
        kCanvas,
        kTranslator);

    ASSERT_GE(panel.lines.size(), 3U);
    EXPECT_EQ(panel.lines[2].text, "people 9/16");
    EXPECT_EQ(
        antwika::game::populationCapacityOf(
            antwika::game::HousingLevel::Hovel),
        16);
}

// Nobody lives in a well, so there is no occupancy to be had.
// The very test the tier is listed on answers for this too.
TEST(ReadoutPanelTest, Panel_SaysNoOccupancyForABuildingNobodyLivesIn)
{
    const auto panel = readoutPanel(
        over(
            BuildingSprite{
                .at = Cell{},
                .kind = BuildingKind::Well,
                .population = 7}),
        kCanvas,
        kTranslator);

    ASSERT_EQ(panel.lines.size(), 8U);
    EXPECT_EQ(panel.lines[0].text, "well");
    EXPECT_EQ(panel.lines[1].text, "staff 0/1");
    EXPECT_EQ(panel.lines[2].text, "resources");
    EXPECT_EQ(panel.lines[4].text, "risk");
}

TEST(ReadoutPanelTest, Panel_ColoursTheMedicineLineOutOfTheServiceTable)
{
    const auto panel = readoutPanel(
        over(
            BuildingSprite{
                .at = Cell{},
                .kind = BuildingKind::Doctor,
                .coverage = {0, kCoverageFull}}),
        kCanvas,
        kTranslator);

    ASSERT_EQ(panel.lines.size(), 8U);
    EXPECT_EQ(panel.lines[3].colour, serviceColour(Service::Health));
    EXPECT_EQ(panel.lines[3].text, "  medicine 100/100");
}

// A ruin says its state and stops.
// The fire or the debris is the one thing a reader can act on.
// The building it was is gone either way.
TEST(ReadoutPanelTest, Panel_NamesAFireStillBurning)
{
    const HoverReadout readout{
        .anchor = Point{.x = 100, .y = 90},
        .ruin = antwika::game::RuinView{
            .at = Cell{.x = 2, .y = 2},
            .kind = BuildingKind::House,
            .state = antwika::game::RuinState::Burning}};

    const auto panel = readoutPanel(readout, kCanvas, kTranslator);

    ASSERT_EQ(panel.lines.size(), 1U);
    EXPECT_EQ(panel.lines[0].text, "on fire");
    EXPECT_EQ(panel.lines[0].colour, kReadoutTitle);
}

TEST(ReadoutPanelTest, Panel_NamesTheDebrisAFireLeaves)
{
    const HoverReadout readout{
        .anchor = Point{.x = 100, .y = 90},
        .ruin = antwika::game::RuinView{
            .at = Cell{.x = 2, .y = 2},
            .kind = BuildingKind::Farm,
            .state = antwika::game::RuinState::Debris}};

    const auto panel = readoutPanel(readout, kCanvas, kTranslator);

    ASSERT_EQ(panel.lines.size(), 1U);
    EXPECT_EQ(panel.lines[0].text, "debris");
}

// A house's stock lines read against its own level's shelf.
// A cottage holds four houses' worth -- see stockCapacityOf().
TEST(ReadoutPanelTest, Panel_SizesAHousesShelfByItsLevel)
{
    const auto panel = readoutPanel(
        over(
            BuildingSprite{
                .at = Cell{},
                .kind = BuildingKind::House,
                .stock = {150, 0, 0},
                .level = antwika::game::HousingLevel::Cottage}),
        kCanvas,
        kTranslator);

    ASSERT_GE(panel.lines.size(), 6U);
    EXPECT_EQ(panel.lines[5].text, "  food 150/400");
}

