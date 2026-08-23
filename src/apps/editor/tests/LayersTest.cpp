#include <gtest/gtest.h>

#include <set>

#include <antwika/map/Layers.hpp>
#include "antwika/editor/ui/ToolPanel.hpp"

using antwika::map::getDefaultLayers;
using antwika::map::kBaseLayer;
using antwika::map::kBaseLayerName;
using antwika::editor::kLayersPanelWidget;
using antwika::editor::kPaletteWidget;
using antwika::editor::kRailWidget;
using antwika::map::kMaxLayers;
using antwika::editor::kStatusBarWidget;
using antwika::editor::kToolPanelWidget;
using antwika::map::getLayerWidget;

TEST(LayersTest, DefaultLayers_HoldTheOneEveryMapHas)
{
    const auto layers = getDefaultLayers();

    ASSERT_EQ(layers.size(), 1U);
    EXPECT_EQ(layers.at(kBaseLayer).name, kBaseLayerName);
}

TEST(LayersTest, LayerWidget_GivesEveryLayerARowOfItsOwn)
{
    std::set<antwika::widget::WidgetId> seenWidgets{
        kToolPanelWidget,
        kStatusBarWidget,
        kLayersPanelWidget,
        kPaletteWidget,
        kRailWidget};

    for (std::size_t index = 0; index < kMaxLayers; ++index)
    {
        EXPECT_TRUE(seenWidgets.insert(getLayerWidget(index)).second);
    }
}

TEST(LayersTest, WithLayerAdded_LaysAnotherOverTheOnesAlready)
{
    const auto layers = antwika::map::getWithLayerAdded(
        antwika::map::getDefaultLayers());

    ASSERT_EQ(layers.size(), 2U);
    EXPECT_EQ(
        layers.front().name, antwika::map::kBaseLayerName);
    EXPECT_NE(layers.back().name, layers.front().name);
}

TEST(LayersTest, WithLayerAdded_NamesTheOneLaidAfterNoOther)
{
    auto layers = antwika::map::getDefaultLayers();

    for (std::size_t index = 0; index < 4; ++index)
    {
        layers = antwika::map::getWithLayerAdded(layers);
    }

    const std::set<std::string> names(
        [&layers]
        {
            std::set<std::string> layerNames;

            for (const auto &layer : layers)
            {
                layerNames.insert(layer.name);
            }

            return layerNames;
        }());

    EXPECT_EQ(names.size(), layers.size());
}

TEST(LayersTest, WithLayerAdded_CallsTheOneLaidDecorAndCounted)
{
    const auto layers = antwika::map::getWithLayerAdded(
        antwika::map::getWithLayerAdded(
            antwika::map::getDefaultLayers()));

    ASSERT_EQ(layers.size(), 3U);
    EXPECT_EQ(layers.at(1).name, "Decor 1");
    EXPECT_EQ(layers.at(2).name, "Decor 2");
}

TEST(LayersTest, LayerLabel_NamesTheBaseAndCountsTheDecor)
{
    EXPECT_EQ(
        antwika::map::getLayerLabel(kBaseLayer), kBaseLayerName);
    EXPECT_EQ(antwika::map::getLayerLabel(1), "Decor 1");
    EXPECT_EQ(antwika::map::getLayerLabel(2), "Decor 2");
}

TEST(LayersTest, WithLayerAdded_LaysNothingOverAMapAlreadyFull)
{
    auto layers = antwika::map::getDefaultLayers();

    while (layers.size() < antwika::map::kMaxLayers)
    {
        layers = antwika::map::getWithLayerAdded(layers);
    }

    EXPECT_EQ(
        antwika::map::getWithLayerAdded(layers).size(),
        antwika::map::kMaxLayers);
}

TEST(LayersTest, WithLayerRemoved_TakesTheOneNamedAway)
{
    const auto layers = antwika::map::getWithLayerAdded(
        antwika::map::getWithLayerAdded(
            antwika::map::getDefaultLayers()));
    const auto left =
        antwika::map::getWithLayerRemoved(layers, 1);

    ASSERT_EQ(left.size(), 2U);
    EXPECT_EQ(left.front().name, layers.front().name);
    EXPECT_EQ(left.back().name, layers.back().name);
}

TEST(LayersTest, WithLayerRemoved_KeepsTheLayerHoldingTheTiles)
{
    const auto layers = antwika::map::getWithLayerAdded(
        antwika::map::getDefaultLayers());

    EXPECT_EQ(
        antwika::map::getWithLayerRemoved(
            layers, antwika::map::kBaseLayer),
        layers);
    EXPECT_EQ(
        antwika::map::getWithLayerRemoved(layers, 9), layers);
}
