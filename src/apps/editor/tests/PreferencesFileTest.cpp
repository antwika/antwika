#include <gtest/gtest.h>

#include <fstream>
#include <string>
#include <string_view>

#include <antwika/testing/ScratchDirectory.hpp>

#include "antwika/editor/Preferences.hpp"
#include "antwika/editor/PreferencesFile.hpp"

using antwika::editor::getLoadPreferences;
using antwika::editor::getPreferencesPath;
using antwika::editor::Preferences;
using antwika::editor::savePreferences;
using antwika::editor::Tool;
using antwika::editor::View;
using antwika::testing::ScratchDirectory;

namespace
{
    [[nodiscard]] Preferences getWidenedPreferences()
    {
        Preferences preferences;
        preferences.tool = Tool::Lamp;
        preferences.view = View::Atlases;
        preferences.panelSizes.toolWidth = 77;
        preferences.panelSizes.entityWidth = 321;
        preferences.panelSizes.inspectWidth = 222;
        preferences.panelSizes.railWidth = 111;
        preferences.panelSizes.cardWidth = 444;
        preferences.panelSizes.planFirstWidth = 555;
        preferences.panelSizes.planSecondWidth = 666;

        return preferences;
    }

    [[nodiscard]] Preferences getUnlitPreferences()
    {
        Preferences preferences;
        preferences.lighting = false;

        return preferences;
    }

    void writeSidecar(
        const std::string &mapPath, const std::string_view text)
    {
        std::ofstream writing(getPreferencesPath(mapPath));
        writing << text;
    }
}

TEST(PreferencesFileTest, LoadPreferences_RestsWhenNoFileIsThere)
{
    const ScratchDirectory scratch("editor-preferences");

    EXPECT_EQ(getLoadPreferences(scratch.pathIn("map.json")), Preferences{});
}

TEST(PreferencesFileTest, SavePreferences_BringsEveryPanelWidthBack)
{
    const ScratchDirectory scratch("editor-preferences");
    const auto mapPath = scratch.pathIn("map.json");

    savePreferences(mapPath, getWidenedPreferences());

    EXPECT_EQ(getLoadPreferences(mapPath), getWidenedPreferences());
}

TEST(PreferencesFileTest, SavePreferences_BringsTheEditorsLightingBack)
{
    const ScratchDirectory scratch("editor-preferences");
    const auto mapPath = scratch.pathIn("map.json");
    auto preferences = getWidenedPreferences();

    preferences.lighting = !preferences.lighting;
    savePreferences(mapPath, preferences);

    EXPECT_EQ(getLoadPreferences(mapPath).lighting, preferences.lighting);
}

TEST(PreferencesFileTest, LoadPreferences_RestsTheLightingThatIsMissing)
{
    const ScratchDirectory scratch("editor-preferences");
    const auto mapPath = scratch.pathIn("map.json");

    writeSidecar(mapPath, "{}");

    EXPECT_EQ(
        getLoadPreferences(mapPath).lighting, Preferences{}.lighting);
    EXPECT_FALSE(
        getLoadPreferences(mapPath, getUnlitPreferences()).lighting);
}

TEST(PreferencesFileTest, LoadPreferences_LetsTheSavedLightingWinOverTheRest)
{
    const ScratchDirectory scratch("editor-preferences");
    const auto mapPath = scratch.pathIn("map.json");

    writeSidecar(mapPath, R"({"lighting": true})");

    EXPECT_TRUE(
        getLoadPreferences(mapPath, getUnlitPreferences()).lighting);
}

TEST(PreferencesFileTest, LoadPreferences_RestsAPanelWidthThatIsMissing)
{
    const ScratchDirectory scratch("editor-preferences");
    const auto mapPath = scratch.pathIn("map.json");

    writeSidecar(mapPath, R"({"tool": "lamp", "railWidth": 111})");

    const auto preferences = getLoadPreferences(mapPath);

    EXPECT_EQ(preferences.panelSizes.railWidth, 111U);
    EXPECT_EQ(preferences.panelSizes.entityWidth, 0U);
}

TEST(PreferencesFileTest, LoadPreferences_RestsAPanelWidthThatIsNotANumber)
{
    const ScratchDirectory scratch("editor-preferences");
    const auto mapPath = scratch.pathIn("map.json");

    writeSidecar(mapPath, R"({"railWidth": "wide"})");

    EXPECT_EQ(
        getLoadPreferences(mapPath).panelSizes.railWidth,
        0U);
}

TEST(PreferencesFileTest, LoadPreferences_RestsAPanelWidthBelowZero)
{
    const ScratchDirectory scratch("editor-preferences");
    const auto mapPath = scratch.pathIn("map.json");

    writeSidecar(mapPath, R"({"railWidth": -20})");

    EXPECT_EQ(
        getLoadPreferences(mapPath).panelSizes.railWidth,
        0U);
}

TEST(PreferencesFileTest, LoadPreferences_RestsAPanelWidthPastAnyWindow)
{
    const ScratchDirectory scratch("editor-preferences");
    const auto mapPath = scratch.pathIn("map.json");

    writeSidecar(mapPath, R"({"railWidth": 999999})");

    EXPECT_EQ(
        getLoadPreferences(mapPath).panelSizes.railWidth,
        0U);
}

TEST(PreferencesFileTest, LoadPreferences_RestsOnAFileThatIsNotJson)
{
    const ScratchDirectory scratch("editor-preferences");
    const auto mapPath = scratch.pathIn("map.json");

    writeSidecar(mapPath, "not json at all");

    EXPECT_EQ(getLoadPreferences(mapPath), Preferences{});
}

TEST(PreferencesFileTest, LoadPreferences_RestsOnAFileThatIsNotAnObject)
{
    const ScratchDirectory scratch("editor-preferences");
    const auto mapPath = scratch.pathIn("map.json");

    writeSidecar(mapPath, "[1, 2, 3]");

    EXPECT_EQ(getLoadPreferences(mapPath), Preferences{});
}
