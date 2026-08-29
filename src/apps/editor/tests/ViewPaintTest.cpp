#include <gtest/gtest.h>

#include <antwika/editor/ui/AtlasSheetsView.hpp>
#include <antwika/editor/ui/CharacterSheetView.hpp>
#include <antwika/editor/ui/GizmoView.hpp>
#include <antwika/editor/ui/IconsView.hpp>
#include <antwika/editor/ui/PlanView.hpp>
#include <antwika/editor/ui/WorldView.hpp>

#include "antwika/editor/Preferences.hpp"

using antwika::editor::AtlasSheetsView;
using antwika::editor::CharacterSheetView;
using antwika::editor::IconsView;
using antwika::editor::PlanView;
using antwika::editor::WorldView;
using antwika::editor::Paint;
using antwika::editor::View;

TEST(ViewPaintTest, OffersPaint_GivesEveryViewTheBrushLineAndFill)
{
    const AtlasSheetsView atlasView;
    const CharacterSheetView characterView;
    const IconsView iconsView;
    const PlanView planView;

    for (const auto paint : {Paint::Brush, Paint::Line, Paint::Fill})
    {
        EXPECT_TRUE(atlasView.offersPaint(paint));
        EXPECT_TRUE(characterView.offersPaint(paint));
        EXPECT_TRUE(iconsView.offersPaint(paint));
        EXPECT_TRUE(planView.offersPaint(paint));
    }
}

TEST(ViewPaintTest, OffersPaint_KeepsTheMarkToTheCharacterSheet)
{
    const CharacterSheetView characterView;
    const AtlasSheetsView atlasView;
    const IconsView iconsView;

    EXPECT_TRUE(characterView.offersPaint(Paint::Select));
    EXPECT_FALSE(atlasView.offersPaint(Paint::Select));
    EXPECT_FALSE(iconsView.offersPaint(Paint::Select));
}

TEST(ViewPaintTest, OffersPaint_KeepsTheRectAndCircleToTheAtlases)
{
    const AtlasSheetsView atlasView;
    const CharacterSheetView characterView;
    const IconsView iconsView;

    for (const auto paint : {Paint::Rect, Paint::Circle})
    {
        EXPECT_TRUE(atlasView.offersPaint(paint));
        EXPECT_FALSE(characterView.offersPaint(paint));
        EXPECT_FALSE(iconsView.offersPaint(paint));
    }
}

TEST(ViewPaintTest, TakesPaintKeys_HoldsForTheTwoSheetsThatPaint)
{
    const AtlasSheetsView atlasView;
    const CharacterSheetView characterView;
    const IconsView iconsView;
    const PlanView planView;

    EXPECT_TRUE(atlasView.takesPaintKeys());
    EXPECT_TRUE(characterView.takesPaintKeys());
    EXPECT_FALSE(iconsView.takesPaintKeys());
    EXPECT_FALSE(planView.takesPaintKeys());
}

TEST(ViewPaintTest, Claims_TakesUpOneTabAndLeavesTheOthers)
{
    const AtlasSheetsView atlasView;
    const CharacterSheetView characterView;
    const IconsView iconsView;
    const antwika::editor::GizmoView gizmoView;
    const PlanView planView;
    const WorldView worldView;

    EXPECT_TRUE(atlasView.claims(View::Atlases, false));
    EXPECT_TRUE(characterView.claims(View::Character, false));
    EXPECT_TRUE(iconsView.claims(View::Icons, false));
    EXPECT_TRUE(gizmoView.claims(View::Gizmos, false));
    EXPECT_TRUE(planView.claims(View::Plan, false));
    EXPECT_TRUE(worldView.claims(View::World, false));

    EXPECT_FALSE(atlasView.claims(View::Character, false));
    EXPECT_FALSE(characterView.claims(View::Atlases, false));
    EXPECT_FALSE(iconsView.claims(View::Plan, false));
    EXPECT_FALSE(gizmoView.claims(View::Icons, false));
    EXPECT_FALSE(planView.claims(View::World, false));
    EXPECT_FALSE(worldView.claims(View::Plan, false));
}

TEST(ViewPaintTest, Claims_GivesPlayToTheWorldWhicheverTabWasUp)
{
    const AtlasSheetsView atlasView;
    const CharacterSheetView characterView;
    const IconsView iconsView;
    const antwika::editor::GizmoView gizmoView;
    const PlanView planView;
    const WorldView worldView;

    for (const auto tab : antwika::editor::kEveryView)
    {
        EXPECT_TRUE(worldView.claims(tab, true));
        EXPECT_FALSE(atlasView.claims(tab, true));
        EXPECT_FALSE(characterView.claims(tab, true));
        EXPECT_FALSE(iconsView.claims(tab, true));
        EXPECT_FALSE(gizmoView.claims(tab, true));
        EXPECT_FALSE(planView.claims(tab, true));
    }
}
