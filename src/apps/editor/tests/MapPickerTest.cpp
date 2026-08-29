#include <gtest/gtest.h>

#include <set>
#include <string>
#include <vector>

#include "antwika/editor/ui/MapPicker.hpp"

#include "antwika/editor/ui/WidgetIds.hpp"

using antwika::editor::kMaxPicked;
using antwika::editor::kMaxPickedRows;
using antwika::editor::getEnsureMapExtension;
using antwika::editor::getFilterMapNames;
using antwika::editor::getMapRowWidget;

TEST(MapPickerTest, FilterMapNames_TakesOnlyWhatIsNamedAsAMap)
{
    const std::vector<std::string> names{
        "keep.json", "atlas-15x9.png", "notes", "map.json.bak"};

    EXPECT_EQ(
        getFilterMapNames(names),
        (std::vector<std::string>{"keep.json"}));
}

TEST(MapPickerTest, FilterMapNames_ListsThemInTheSameOrderEveryTime)
{
    const std::vector<std::string> names{
        "second.json", "first.json", "third.json"};

    EXPECT_EQ(
        getFilterMapNames(names),
        (std::vector<std::string>{
            "first.json", "second.json", "third.json"}));
}

TEST(MapPickerTest, FilterMapNames_ListsNoMoreThanThePickerHasRoomFor)
{
    std::vector<std::string> names;

    for (std::size_t index = 0; index < kMaxPicked + 8; ++index)
    {
        names.push_back(
            "map" + std::to_string(100 + index) + ".json");
    }

    EXPECT_EQ(getFilterMapNames(names).size(), kMaxPicked);
}

TEST(MapPickerTest, FilterMapNames_TakesNothingFromAnEmptyFolder)
{
    EXPECT_TRUE(getFilterMapNames(std::vector<std::string>{}).empty());
}

TEST(MapPickerTest, EnsureMapExtension_PutsTheSuffixOnAName)
{
    EXPECT_EQ(getEnsureMapExtension("keep"), "keep.json");
}

TEST(MapPickerTest, EnsureMapExtension_LeavesANameThatHasItAlone)
{
    EXPECT_EQ(getEnsureMapExtension("keep.json"), "keep.json");
    EXPECT_EQ(getEnsureMapExtension(".json"), ".json.json");
}

TEST(MapPickerTest, MapRowWidget_GivesEveryListedNameOneOfItsOwn)
{
    std::set<antwika::widget::WidgetId> seenWidgets;

    for (std::size_t index = 0; index < kMaxPicked; ++index)
    {
        EXPECT_TRUE(seenWidgets.insert(getMapRowWidget(index)).second);
    }

    EXPECT_FALSE(seenWidgets.contains(antwika::editor::kPickerNameWidget));
    EXPECT_FALSE(seenWidgets.contains(antwika::editor::kPickerConfirmWidget));
    EXPECT_FALSE(seenWidgets.contains(antwika::editor::kPickerCancelWidget));
    EXPECT_FALSE(
        seenWidgets.contains(antwika::editor::kPickerParentFolderWidget));
}

TEST(MapPickerTest, MapRowWidget_KeepsMapRowsApartFromAFullFolderList)
{
    std::set<antwika::widget::WidgetId> seenWidgets;

    for (std::size_t index = 0; index < kMaxPickedRows; ++index)
    {
        EXPECT_TRUE(seenWidgets.insert(getMapRowWidget(index)).second);
    }

    EXPECT_NE(getMapRowWidget(kMaxPicked), getMapRowWidget(0));
    EXPECT_FALSE(
        seenWidgets.contains(antwika::editor::kFirstCharacterWidget));
}
