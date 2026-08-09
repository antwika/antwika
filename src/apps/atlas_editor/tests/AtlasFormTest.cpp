#include <gtest/gtest.h>

#include <cstddef>
#include <string>

#include "antwika/atlas_editor/AtlasForm.hpp"
#include "antwika/atlas_editor/AtlasMeta.hpp"
#include "antwika/atlas_editor/MessageId.hpp"

using antwika::atlas_editor::AtlasField;
using antwika::atlas_editor::AtlasForm;
using antwika::atlas_editor::AtlasKind;
using antwika::atlas_editor::AtlasMeta;
using antwika::atlas_editor::fieldNameId;
using antwika::atlas_editor::formIsWhole;
using antwika::atlas_editor::formOf;
using antwika::atlas_editor::kAtlasFieldCount;
using antwika::atlas_editor::kAtlasPresetCount;
using antwika::atlas_editor::presetForm;
using antwika::atlas_editor::presetNameId;
using antwika::atlas_editor::MessageId;
using antwika::atlas_editor::metaOf;
using antwika::gfx::Point;
using antwika::gfx::Size;

namespace
{
    AtlasMeta described()
    {
        return AtlasMeta{
            .kind = AtlasKind::Isometric,
            .columns = 8,
            .rows = 6,
            .sprite = {.width = 64, .height = 96},
            .pivot = {.x = 32, .y = 64},
            .isometric = {.width = 32, .height = 16}};
    }

    void fill(
        AtlasForm &form, const AtlasField field, const std::string &text)
    {
        form.values[static_cast<std::size_t>(field)] = text;
    }
}

TEST(AtlasFormTest, MetaOf_ReadsBackTheAtlasFormOfWroteOut)
{
    EXPECT_EQ(metaOf(formOf(described())), described());
}

TEST(AtlasFormTest, FormOf_CarriesTheKindStraightThrough)
{
    auto flat = described();
    flat.kind = AtlasKind::Flat;

    EXPECT_EQ(formOf(flat).kind, AtlasKind::Flat);
}

TEST(AtlasFormTest, MetaOf_CountsAFieldThatIsNotAWholeNumberAsZero)
{
    auto form = formOf(described());
    fill(form, AtlasField::Columns, "eight");

    EXPECT_EQ(metaOf(form).columns, 0U);
}

TEST(AtlasFormTest, MetaOf_CountsAnEmptyFieldAsZero)
{
    auto form = formOf(described());
    fill(form, AtlasField::Rows, "");

    EXPECT_EQ(metaOf(form).rows, 0U);
}

TEST(AtlasFormTest, MetaOf_CapsAFieldThatRunsPastTheLimit)
{
    auto form = formOf(described());
    fill(form, AtlasField::SpriteWidth, "99999999999");

    EXPECT_EQ(metaOf(form).sprite.width, 1U << 16U);
}

TEST(AtlasFormTest, FormIsWhole_AcceptsAFormThatNamesEveryExtent)
{
    EXPECT_TRUE(formIsWhole(formOf(described())));
}

TEST(AtlasFormTest, FormIsWhole_RefusesASheetWithNoSlotsInIt)
{
    auto form = formOf(described());
    fill(form, AtlasField::Columns, "0");

    EXPECT_FALSE(formIsWhole(form));

    form = formOf(described());
    fill(form, AtlasField::Rows, "0");

    EXPECT_FALSE(formIsWhole(form));
}

TEST(AtlasFormTest, FormIsWhole_RefusesASlotWithNoExtent)
{
    auto form = formOf(described());
    fill(form, AtlasField::SpriteWidth, "0");

    EXPECT_FALSE(formIsWhole(form));

    form = formOf(described());
    fill(form, AtlasField::SpriteHeight, "0");

    EXPECT_FALSE(formIsWhole(form));
}

TEST(AtlasFormTest, FieldNameId_NamesEveryFieldOfItsOwn)
{
    for (std::size_t field = 0; field < kAtlasFieldCount; ++field)
    {
        for (std::size_t other = field + 1; other < kAtlasFieldCount;
             ++other)
        {
            EXPECT_NE(
                fieldNameId(static_cast<AtlasField>(field)),
                fieldNameId(static_cast<AtlasField>(other)));
        }
    }
}

TEST(AtlasFormTest, MetaOf_CountsAFieldBelowTheDigitsAsZero)
{
    auto form = formOf(described());
    fill(form, AtlasField::PivotX, "-4");

    EXPECT_EQ(metaOf(form).pivot.x, 0);
}

TEST(AtlasFormTest, OperatorEquals_ComparesTheKindAndEveryField)
{
    const auto form = formOf(described());

    EXPECT_EQ(form, formOf(described()));

    auto turned = form;
    turned.kind = AtlasKind::Flat;

    EXPECT_NE(form, turned);

    for (std::size_t field = 0; field < kAtlasFieldCount; ++field)
    {
        auto changed = form;
        changed.values[field] += "1";

        EXPECT_NE(form, changed) << field;
    }
}

TEST(AtlasFormTest, PresetForm_MatchesTheSheetsTheGameShips)
{
    EXPECT_EQ(
        metaOf(presetForm(0)),
        (AtlasMeta{
            .kind = AtlasKind::Isometric,
            .columns = 8,
            .rows = 8,
            .sprite = {.width = 64, .height = 96},
            .pivot = {.x = 32, .y = 64},
            .isometric = {.width = 32, .height = 16}}));

    EXPECT_EQ(
        metaOf(presetForm(1)),
        (AtlasMeta{
            .kind = AtlasKind::Isometric,
            .columns = 8,
            .rows = 8,
            .sprite = {.width = 96, .height = 112},
            .pivot = {.x = 48, .y = 80},
            .isometric = {.width = 64, .height = 32}}));

    EXPECT_EQ(
        metaOf(presetForm(2)),
        (AtlasMeta{
            .kind = AtlasKind::Isometric,
            .columns = 8,
            .rows = 8,
            .sprite = {.width = 128, .height = 128},
            .pivot = {.x = 64, .y = 96},
            .isometric = {.width = 96, .height = 48}}));
}

TEST(AtlasFormTest, PresetForm_WidensTheDiamondByOneCellEachStep)
{
    EXPECT_EQ(
        metaOf(presetForm(3)).isometric,
        (Size{.width = 128, .height = 64}));
    EXPECT_EQ(
        metaOf(presetForm(3)).sprite,
        (Size{.width = 160, .height = 144}));
}

TEST(AtlasFormTest, PresetForm_WrapsAPresetPastTheLastOne)
{
    EXPECT_EQ(presetForm(kAtlasPresetCount), presetForm(0));
}

TEST(AtlasFormTest, PresetNameId_NamesEveryPresetOfItsOwn)
{
    for (std::size_t preset = 0; preset < kAtlasPresetCount; ++preset)
    {
        for (std::size_t other = preset + 1; other < kAtlasPresetCount;
             ++other)
        {
            EXPECT_NE(presetNameId(preset), presetNameId(other));
        }
    }
}
