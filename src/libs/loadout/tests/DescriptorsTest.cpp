#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <format>
#include <set>
#include <string>
#include <variant>

#include "antwika/loadout/ComponentRow.hpp"
#include "antwika/loadout/ComponentValue.hpp"
#include "antwika/loadout/Descriptors.hpp"
#include "antwika/loadout/FieldKind.hpp"
#include "antwika/loadout/FieldRow.hpp"
#include "antwika/loadout/LoadoutError.hpp"

using antwika::loadout::ComponentValue;
using antwika::loadout::FieldKind;
using antwika::loadout::FieldRow;
using antwika::loadout::getComponentRow;
using antwika::loadout::getComponentRows;
using antwika::loadout::LoadoutError;

namespace
{
    [[nodiscard]] nlohmann::json probeOf(const FieldRow &field)
    {
        switch (field.kind)
        {
        case FieldKind::Flag:
            return nlohmann::json(true);
        case FieldKind::Whole:
            return nlohmann::json(
                static_cast<std::int64_t>(field.least) + 1);
        case FieldKind::Fixed:
            return nlohmann::json(field.least + 1.5);
        case FieldKind::Tint:
        case FieldKind::Slots:
            return nlohmann::json::array({1, 2, 3, 4});
        }

        return {};
    } // GCOVR_EXCL_LINE

    [[nodiscard]] nlohmann::json mistypedOf(const FieldRow &field)
    {
        switch (field.kind)
        {
        case FieldKind::Flag:
            return nlohmann::json(1);
        case FieldKind::Whole:
            return nlohmann::json(1.5);
        case FieldKind::Fixed:
            return nlohmann::json("x");
        case FieldKind::Tint:
        case FieldKind::Slots:
            return nlohmann::json::array({true, 2, 3, 4});
        }

        return {};
    } // GCOVR_EXCL_LINE

    [[nodiscard]] std::string gibberishOf(const FieldRow &field)
    {
        switch (field.kind)
        {
        case FieldKind::Flag:
            return "maybe";
        case FieldKind::Whole:
        case FieldKind::Fixed:
            return "ten";
        case FieldKind::Tint:
            return "01020G";
        case FieldKind::Slots:
            return "1 2 3";
        }

        return {};
    } // GCOVR_EXCL_LINE
}

TEST(DescriptorsTest, ComponentRows_CoverEveryVariantAlternative)
{
    EXPECT_EQ(
        getComponentRows().size(),
        std::variant_size_v<ComponentValue>);
}

TEST(DescriptorsTest, ComponentRows_CarryUniqueNames)
{
    std::set<std::string> names;

    for (const auto &row : getComponentRows())
    {
        EXPECT_FALSE(row.name.empty());

        names.insert(std::string(row.name));
    }

    EXPECT_EQ(names.size(), getComponentRows().size());
}

TEST(DescriptorsTest, ComponentRows_FreshHoldsTheNamedAlternative)
{
    std::size_t alternative = 0;

    for (const auto &row : getComponentRows())
    {
        SCOPED_TRACE(std::string(row.name));

        EXPECT_EQ(row.fresh().index(), alternative);

        alternative += 1;
    }
}

TEST(DescriptorsTest, GetComponentRow_AnswersTheRowNamed)
{
    for (const auto &row : getComponentRows())
    {
        const auto *const foundRow = getComponentRow(row.name);

        ASSERT_NE(foundRow, nullptr);
        EXPECT_EQ(foundRow->name, row.name);
    }
}

TEST(DescriptorsTest, GetComponentRow_AnswersNothingForAStrangeName)
{
    EXPECT_EQ(getComponentRow("component::Stranger"), nullptr);
}

TEST(DescriptorsTest, Fields_RoundTripANonDefaultValueThroughJson)
{
    for (const auto &row : getComponentRows())
    {
        for (const auto &field : row.fields)
        {
            SCOPED_TRACE(
                std::string(row.name) + "." + std::string(field.key));

            auto value = row.fresh();

            const auto probe = probeOf(field);

            field.setFrom(value, probe);

            EXPECT_EQ(field.valueOf(value), probe);
        }
    }
}

TEST(DescriptorsTest, Fields_ClampWhatJsonBringsOutOfBounds)
{
    for (const auto &row : getComponentRows())
    {
        for (const auto &field : row.fields)
        {
            if (field.kind == FieldKind::Flag)
            {
                continue;
            }

            SCOPED_TRACE(
                std::string(row.name) + "." + std::string(field.key));

            auto value = row.fresh();

            if (field.kind == FieldKind::Tint
                || field.kind == FieldKind::Slots)
            {
                field.setFrom(
                    value, nlohmann::json::array({-1, 300, 0, 255}));

                EXPECT_EQ(
                    field.valueOf(value),
                    nlohmann::json::array({0, 255, 0, 255}));

                continue;
            }

            const bool wholeKind = field.kind == FieldKind::Whole;

            field.setFrom(
                value,
                wholeKind ? nlohmann::json(
                            static_cast<std::int64_t>(field.least) - 1)
                          : nlohmann::json(field.least - 1.0));

            EXPECT_EQ(
                field.valueOf(value).get<double>(), field.least);

            field.setFrom(
                value,
                wholeKind ? nlohmann::json(
                            static_cast<std::int64_t>(field.most) + 1)
                          : nlohmann::json(field.most + 1.0));

            EXPECT_EQ(field.valueOf(value).get<double>(), field.most);
        }
    }
}

TEST(DescriptorsTest, Fields_RefuseAMistypedJsonValue)
{
    for (const auto &row : getComponentRows())
    {
        for (const auto &field : row.fields)
        {
            SCOPED_TRACE(
                std::string(row.name) + "." + std::string(field.key));

            auto value = row.fresh();

            EXPECT_THROW(
                field.setFrom(value, mistypedOf(field)), LoadoutError);
        }
    }
}

TEST(DescriptorsTest, Fields_RefuseAJsonListOfTheWrongLength)
{
    for (const auto &row : getComponentRows())
    {
        for (const auto &field : row.fields)
        {
            if (field.kind != FieldKind::Tint
                && field.kind != FieldKind::Slots)
            {
                continue;
            }

            SCOPED_TRACE(
                std::string(row.name) + "." + std::string(field.key));

            auto value = row.fresh();

            EXPECT_THROW(
                field.setFrom(value, nlohmann::json::array({1})),
                LoadoutError);
        }
    }
}

TEST(DescriptorsTest, Fields_RoundTripANonDefaultValueThroughText)
{
    for (const auto &row : getComponentRows())
    {
        for (const auto &field : row.fields)
        {
            SCOPED_TRACE(
                std::string(row.name) + "." + std::string(field.key));

            auto value = row.fresh();

            field.setFrom(value, probeOf(field));

            const auto text = field.textOf(value);

            auto rebornValue = row.fresh();

            EXPECT_TRUE(field.setFromText(rebornValue, text));
            EXPECT_EQ(field.textOf(rebornValue), text);
        }
    }
}

TEST(DescriptorsTest, Fields_RefuseTextThatDoesNotParse)
{
    for (const auto &row : getComponentRows())
    {
        for (const auto &field : row.fields)
        {
            SCOPED_TRACE(
                std::string(row.name) + "." + std::string(field.key));

            auto value = row.fresh();

            const auto keptJson = field.valueOf(value);

            EXPECT_FALSE(field.setFromText(value, gibberishOf(field)));
            EXPECT_FALSE(field.setFromText(value, ""));
            EXPECT_EQ(field.valueOf(value), keptJson);
        }
    }
}

TEST(DescriptorsTest, Fields_ClampNumbersBroughtAsText)
{
    for (const auto &row : getComponentRows())
    {
        for (const auto &field : row.fields)
        {
            if (field.kind != FieldKind::Whole
                && field.kind != FieldKind::Fixed)
            {
                continue;
            }

            SCOPED_TRACE(
                std::string(row.name) + "." + std::string(field.key));

            auto value = row.fresh();

            const auto text =
                field.kind == FieldKind::Whole
                    ? std::format(
                          "{}",
                          static_cast<std::int64_t>(field.most) + 1)
                    : std::format("{}", field.most + 1.0);

            EXPECT_TRUE(field.setFromText(value, text));
            EXPECT_EQ(field.valueOf(value).get<double>(), field.most);
        }
    }
}
