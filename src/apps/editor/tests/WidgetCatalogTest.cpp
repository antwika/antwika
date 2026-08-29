#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include <antwika/decor/Decor.hpp>
#include <antwika/gfx/NullBackend.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/NullInputBackend.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/map/Layers.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/ui/Interactions.hpp>

#include "antwika/editor/Editor.hpp"
#include "antwika/editor/fakes/EditorProbe.hpp"
#include "antwika/editor/ui/WidgetCatalog.hpp"
#include "antwika/editor/ui/WidgetIds.hpp"

using antwika::editor::Editor;
using antwika::editor::fakes::EditorProbe;
using antwika::editor::widget_catalog::carryFamilyEdit;
using antwika::editor::widget_catalog::Catalog;
using antwika::editor::widget_catalog::hintIn;
using antwika::editor::widget_catalog::isEveryFieldFamilyApart;
using antwika::editor::widget_catalog::isEveryFieldFamilyClaimed;
using antwika::editor::widget_catalog::isOnToolPanel;
using antwika::editor::widget_catalog::placeEndIn;
using antwika::gfx::NullBackend;
using antwika::input::NullInputBackend;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{

    constexpr std::string_view kMissingMapPath =
        "assets/maps/no-such-map.json";

    [[nodiscard]] antwika::widget::WidgetId getProbePlaceWidget(
        const std::size_t place)
    {
        return antwika::editor::getWidgetAfter(
            antwika::widget::kNoWidget, 900 + place);
    }

    [[nodiscard]] antwika::widget::WidgetId getFarPlaceWidget(
        const std::size_t place)
    {
        return antwika::editor::getWidgetAfter(
            antwika::widget::kNoWidget, 950 + place);
    }

    [[nodiscard]] antwika::widget::WidgetId getFoldedPlaceWidget(
        const std::size_t)
    {
        return getProbePlaceWidget(0);
    }

    struct FamilyEditRecord final
    {
        std::size_t timesTold = 0;

        std::size_t place = 0;

        std::string text;
    };

    FamilyEditRecord familyEditRecord;

    class WidgetCatalogTest : public ::testing::Test
    {
    protected:
        NiceMock<MockLogger> logger;
        NullBackend backend{logger};
        NullInputBackend inputs{logger};
    };

}

TEST_F(WidgetCatalogTest, PlaceEndIn_LetsADynamicCountWinOverTheFixedOne)
{
    const Editor editor(
        logger, backend, inputs, std::string(kMissingMapPath));

    const Catalog::FamilyRow grownFamilyRow{
        .widgetAt = getProbePlaceWidget,
        .placeCount = 5,
        .placeCountOf = [](const Editor &) { return std::size_t{2}; }};
    const Catalog::FamilyRow fixedFamilyRow{
        .widgetAt = getProbePlaceWidget, .placeCount = 5};

    EXPECT_EQ(placeEndIn(grownFamilyRow, editor), 2U);
    EXPECT_EQ(placeEndIn(fixedFamilyRow, editor), 5U);
}

TEST_F(WidgetCatalogTest, PlaceEndIn_CountsTheDynamicPlacesFromTheFirst)
{
    const Editor editor(
        logger, backend, inputs, std::string(kMissingMapPath));

    const Catalog::FamilyRow latterFamilyRow{
        .widgetAt = getProbePlaceWidget,
        .firstPlace = 2,
        .placeCountOf = [](const Editor &) { return std::size_t{3}; }};

    EXPECT_EQ(placeEndIn(latterFamilyRow, editor), 5U);
}

TEST_F(WidgetCatalogTest, IsOnToolPanel_ReachesTheLastPlaceOfALatterFamily)
{
    const Editor editor(
        logger, backend, inputs, std::string(kMissingMapPath));

    const std::array familyRows{
        Catalog::FamilyRow{
            .widgetAt = getProbePlaceWidget,
            .firstPlace = 2,
            .placeCountOf = [](const Editor &)
            { return std::size_t{3}; },
            .toolPanelMembership = true}};
    const Catalog catalog{
        .soloRows = {},
        .familyRows = familyRows,
        .sliderRows = {},
        .fieldRows = {},
        .fieldFamilies = {}};

    EXPECT_TRUE(isOnToolPanel(catalog, editor, getProbePlaceWidget(2)));
    EXPECT_TRUE(isOnToolPanel(catalog, editor, getProbePlaceWidget(4)));
    EXPECT_FALSE(isOnToolPanel(catalog, editor, getProbePlaceWidget(1)));
    EXPECT_FALSE(isOnToolPanel(catalog, editor, getProbePlaceWidget(5)));
}

TEST_F(WidgetCatalogTest, HintIn_LetsAPlaceHintWinOverTheSharedOne)
{
    const Catalog::FamilyRow toldFamilyRow{
        .widgetAt = getProbePlaceWidget,
        .placeCount = 2,
        .hint = "shared",
        .hintAt = [](const std::size_t place)
        {
            return place == 0 ? std::string_view{"first"}
                              : std::string_view{"second"};
        }};
    const Catalog::FamilyRow quietFamilyRow{
        .widgetAt = getProbePlaceWidget,
        .placeCount = 2,
        .hint = "shared"};

    EXPECT_EQ(hintIn(toldFamilyRow, 1), "second");
    EXPECT_EQ(hintIn(quietFamilyRow, 1), "shared");
}

TEST_F(WidgetCatalogTest, IsOnToolPanel_AsksADynamicFamilyForItsPlaces)
{
    const Editor editor(
        logger, backend, inputs, std::string(kMissingMapPath));

    const std::array familyRows{
        Catalog::FamilyRow{
            .widgetAt = getProbePlaceWidget,
            .placeCountOf = [](const Editor &)
            { return std::size_t{3}; },
            .toolPanelMembership = true}};
    const Catalog catalog{
        .soloRows = {},
        .familyRows = familyRows,
        .sliderRows = {},
        .fieldRows = {},
        .fieldFamilies = {}};

    EXPECT_TRUE(isOnToolPanel(catalog, editor, getProbePlaceWidget(2)));
    EXPECT_FALSE(isOnToolPanel(catalog, editor, getProbePlaceWidget(3)));
}

TEST_F(WidgetCatalogTest, CarryFamilyEdit_HandsTheEditToTheMatchingPlace)
{
    Editor editor(logger, backend, inputs, std::string(kMissingMapPath));

    familyEditRecord = {};

    const std::array fieldFamilies{
        Catalog::FieldFamilyRow{
            .widgetAt = getProbePlaceWidget,
            .placeCount = 3,
            .editEffect = [](Editor &,
                             const std::size_t place,
                             const std::string &text)
            {
                ++familyEditRecord.timesTold;
                familyEditRecord.place = place;
                familyEditRecord.text = text;
            }}};
    const Catalog catalog{
        .soloRows = {},
        .familyRows = {},
        .sliderRows = {},
        .fieldRows = {},
        .fieldFamilies = fieldFamilies};

    carryFamilyEdit(catalog, editor, getProbePlaceWidget(2), "typed");

    EXPECT_EQ(familyEditRecord.timesTold, 1U);
    EXPECT_EQ(familyEditRecord.place, 2U);
    EXPECT_EQ(familyEditRecord.text, "typed");
}

TEST_F(WidgetCatalogTest, CarryFamilyEdit_LeavesAWidgetPastTheDomainUntold)
{
    Editor editor(logger, backend, inputs, std::string(kMissingMapPath));

    familyEditRecord = {};

    const std::array fieldFamilies{
        Catalog::FieldFamilyRow{
            .widgetAt = getProbePlaceWidget,
            .placeCount = 3,
            .editEffect = [](Editor &,
                             const std::size_t,
                             const std::string &)
            { ++familyEditRecord.timesTold; }}};
    const Catalog catalog{
        .soloRows = {},
        .familyRows = {},
        .sliderRows = {},
        .fieldRows = {},
        .fieldFamilies = fieldFamilies};

    carryFamilyEdit(catalog, editor, getProbePlaceWidget(3), "typed");

    EXPECT_EQ(familyEditRecord.timesTold, 0U);
}

TEST_F(WidgetCatalogTest, IsEveryFieldFamilyClaimed_AsksForWidgetsAndEffect)
{
    const std::array claimedFamilies{
        Catalog::FieldFamilyRow{
            .widgetAt = getProbePlaceWidget,
            .placeCount = 2,
            .editEffect = [](Editor &,
                             const std::size_t,
                             const std::string &) {}}};
    const std::array unclaimedFamilies{
        Catalog::FieldFamilyRow{
            .widgetAt = getProbePlaceWidget, .placeCount = 2}};

    EXPECT_TRUE(isEveryFieldFamilyClaimed(claimedFamilies));
    EXPECT_FALSE(isEveryFieldFamilyClaimed(unclaimedFamilies));
}

TEST_F(WidgetCatalogTest, IsEveryFieldFamilyApart_SpotsAnOverlapWithASoloField)
{
    const std::array fieldFamilies{
        Catalog::FieldFamilyRow{
            .widgetAt = getProbePlaceWidget, .placeCount = 2}};
    const std::array apartFieldRows{
        Catalog::FieldRow{.widget = getProbePlaceWidget(5)}};
    const std::array overlappingFieldRows{
        Catalog::FieldRow{.widget = getProbePlaceWidget(1)}};

    EXPECT_TRUE(isEveryFieldFamilyApart(fieldFamilies, apartFieldRows));
    EXPECT_FALSE(
        isEveryFieldFamilyApart(fieldFamilies, overlappingFieldRows));
}

TEST_F(WidgetCatalogTest, IsEveryFieldFamilyApart_SpotsFamiliesStandingTogether)
{
    const std::array<Catalog::FieldRow, 0> fieldRows{};

    const std::array apartFamilies{
        Catalog::FieldFamilyRow{
            .widgetAt = getProbePlaceWidget, .placeCount = 2},
        Catalog::FieldFamilyRow{
            .widgetAt = getFarPlaceWidget, .placeCount = 2}};
    const std::array sharedFamilies{
        Catalog::FieldFamilyRow{
            .widgetAt = getProbePlaceWidget, .placeCount = 2},
        Catalog::FieldFamilyRow{
            .widgetAt = getProbePlaceWidget, .placeCount = 2}};
    const std::array foldedFamilies{
        Catalog::FieldFamilyRow{
            .widgetAt = getFoldedPlaceWidget, .placeCount = 2}};

    EXPECT_TRUE(isEveryFieldFamilyApart(apartFamilies, fieldRows));
    EXPECT_FALSE(isEveryFieldFamilyApart(sharedFamilies, fieldRows));
    EXPECT_FALSE(isEveryFieldFamilyApart(foldedFamilies, fieldRows));
}

TEST_F(WidgetCatalogTest, BeginSliderDrag_EnsuresTheDecorBeforeTheSlideEffect)
{
    Editor editor(logger, backend, inputs, std::string(kMissingMapPath));
    EditorProbe probe(editor);

    probe.document.map.layers =
        antwika::map::getWithLayerAdded(probe.document.map.layers);
    probe.chosenLayer = probe.document.map.layers.size() - 1;
    probe.stroke.selectedTile = antwika::tilemap::Tile{
        .atlas = antwika::tilemap::Atlas::Wall, .index = 0};

    ASSERT_EQ(
        antwika::decor::decorOf(
            probe.document.map.decor, *probe.stroke.selectedTile),
        nullptr);

    const auto consumedDrag = probe.beginSliderDrag(
        antwika::ui::Interactions{
            .slidChange = antwika::ui::SliderChange{
                .sliderWidget = antwika::editor::kFrequencyWidget,
                .value = 7}});

    EXPECT_TRUE(consumedDrag);

    const auto *decor = antwika::decor::decorOf(
        probe.document.map.decor, *probe.stroke.selectedTile);

    ASSERT_NE(decor, nullptr);
    EXPECT_EQ(decor->frequency, 7U);
}

TEST_F(WidgetCatalogTest, ValueOf_AnswersZeroWhereNoDecorRecordStands)
{
    Editor editor(logger, backend, inputs, std::string(kMissingMapPath));
    EditorProbe probe(editor);

    probe.document.map.layers =
        antwika::map::getWithLayerAdded(probe.document.map.layers);
    probe.chosenLayer = probe.document.map.layers.size() - 1;
    probe.stroke.selectedTile = antwika::tilemap::Tile{
        .atlas = antwika::tilemap::Atlas::Wall, .index = 0};

    ASSERT_EQ(
        antwika::decor::decorOf(
            probe.document.map.decor, *probe.stroke.selectedTile),
        nullptr);

    for (const auto &row : EditorProbe::getCatalog().sliderRows)
    {
        if (!row.decorNeed)
        {
            continue;
        }

        EXPECT_EQ(row.valueOf(editor), 0U);
    }
}

TEST_F(WidgetCatalogTest, ValueOf_AnswersZeroWithNoTileChosen)
{
    Editor editor(logger, backend, inputs, std::string(kMissingMapPath));
    EditorProbe probe(editor);

    probe.stroke.selectedTile.reset();

    for (const auto &row : EditorProbe::getCatalog().sliderRows)
    {
        if (!row.decorNeed)
        {
            continue;
        }

        EXPECT_EQ(row.valueOf(editor), 0U);
    }
}

TEST_F(WidgetCatalogTest, OnScrolled_EnsuresTheDecorBeforeTheWheelNudge)
{
    Editor editor(logger, backend, inputs, std::string(kMissingMapPath));
    EditorProbe probe(editor);

    probe.document.map.layers =
        antwika::map::getWithLayerAdded(probe.document.map.layers);
    probe.chosenLayer = probe.document.map.layers.size() - 1;
    probe.stroke.selectedTile = antwika::tilemap::Tile{
        .atlas = antwika::tilemap::Atlas::Wall, .index = 0};
    probe.pointer.hoveredWidget = antwika::editor::kFrequencyWidget;

    probe.onScrolled(antwika::input::PointerScrolled{.vertical = -1});

    const auto *decor = antwika::decor::decorOf(
        probe.document.map.decor, *probe.stroke.selectedTile);

    ASSERT_NE(decor, nullptr);
    EXPECT_EQ(decor->frequency, antwika::decor::kFullFrequency - 1);
}
