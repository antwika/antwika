#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstddef>
#include <string>
#include <tuple>
#include <variant>

#include <antwika/component/CarriedLight.hpp>
#include <antwika/component/Health.hpp>
#include <antwika/component/Inventory.hpp>
#include <antwika/gfx/Color.hpp>

#include "antwika/loadout/ComponentRow.hpp"
#include "antwika/loadout/ComponentValue.hpp"
#include "antwika/loadout/Descriptors.hpp"
#include "antwika/loadout/Role.hpp"
#include "antwika/loadout/LoadoutError.hpp"
#include "antwika/loadout/ComponentValuesJson.hpp"

using antwika::loadout::getComponentRows;
using antwika::loadout::getReadComponentValues;
using antwika::loadout::getComponentValuesShape;
using antwika::loadout::getWrittenComponentValues;
using antwika::loadout::Role;
using antwika::loadout::LoadoutError;
using antwika::loadout::ComponentValues;

namespace
{
    [[nodiscard]] ComponentValues getMixedValues()
    {
        ComponentValues componentValues;

        componentValues.insert_or_assign(
            "component::Health",
            antwika::component::Health{.food = 12, .water = 34});

        componentValues.insert_or_assign(
            "component::Inventory",
            antwika::component::Inventory{.slots = {1, 2, 3, 4}});

        componentValues.insert_or_assign(
            "component::CarriedLight",
            antwika::component::CarriedLight{
                .tintColor =
                    antwika::gfx::Color{
                        .red = 9, .green = 8, .blue = 7, .alpha = 255},
                .aboveHeight = 1.5F,
                .reach = 24.0F});

        return componentValues;
    } // GCOVR_EXCL_LINE

    [[nodiscard]] std::size_t getTunedRowCount()
    {
        std::size_t count = 0;

        for (const auto &row : getComponentRows())
        {
            if (row.role == Role::Valued)
            {
                count += 1;
            }
        }

        return count;
    }
}

TEST(ComponentValuesJsonTest, ValuesShape_IsAClosedObject)
{
    const auto shape = getComponentValuesShape();

    EXPECT_EQ(shape.at("type"), "object");
    EXPECT_EQ(shape.at("additionalProperties"), false);
    EXPECT_FALSE(shape.contains("required"));
}

TEST(ComponentValuesJsonTest, ValuesShape_NamesExactlyTheValuedRows)
{
    const auto shape = getComponentValuesShape();

    EXPECT_EQ(shape.at("properties").size(), getTunedRowCount());

    for (const auto &row : getComponentRows())
    {
        SCOPED_TRACE(std::string(row.name));

        EXPECT_EQ(
            shape.at("properties").contains(std::string(row.name)),
            row.role == Role::Valued);
    }
}

TEST(ComponentValuesJsonTest, ValuesShape_ClosesEachRowOverAllItsFields)
{
    const auto shape = getComponentValuesShape();

    for (const auto &row : getComponentRows())
    {
        if (row.role != Role::Valued)
        {
            continue;
        }

        SCOPED_TRACE(std::string(row.name));

        const auto &rowShape =
            shape.at("properties").at(std::string(row.name));

        EXPECT_EQ(rowShape.at("type"), "object");
        EXPECT_EQ(rowShape.at("additionalProperties"), false);
        EXPECT_EQ(rowShape.at("properties").size(), row.fields.size());
        EXPECT_EQ(rowShape.at("required").size(), row.fields.size());

        for (const auto &field : row.fields)
        {
            SCOPED_TRACE(std::string(field.key));

            EXPECT_TRUE(
                rowShape.at("properties")
                    .contains(std::string(field.key)));
        }
    }
}

TEST(ComponentValuesJsonTest, WrittenValues_CarryEveryFieldOfEveryEntry)
{
    const auto valuesJson = getWrittenComponentValues(getMixedValues());

    EXPECT_EQ(valuesJson.size(), 3);
    EXPECT_EQ(valuesJson.at("component::Health").at("food"), 12);
    EXPECT_EQ(valuesJson.at("component::Health").at("water"), 34);
    EXPECT_EQ(
        valuesJson.at("component::Inventory").at("slots"),
        nlohmann::json::array({1, 2, 3, 4}));
    EXPECT_EQ(
        valuesJson.at("component::CarriedLight").at("tint"),
        nlohmann::json::array({9, 8, 7, 255}));
    EXPECT_EQ(
        valuesJson.at("component::CarriedLight").at("above"), 1.5);
    EXPECT_EQ(
        valuesJson.at("component::CarriedLight").at("reach"), 24.0);
}

TEST(ComponentValuesJsonTest, WrittenValues_RoundTripThroughReadValues)
{
    const auto componentValues = getMixedValues();

    EXPECT_EQ(getReadComponentValues(getWrittenComponentValues(componentValues)), componentValues);
}

TEST(ComponentValuesJsonTest, WrittenValues_RefuseAStrangeName)
{
    ComponentValues componentValues;

    componentValues.insert_or_assign(
        "component::Stranger", antwika::component::Health{});

    EXPECT_THROW(
        { std::ignore = getWrittenComponentValues(componentValues); }, LoadoutError);
}

TEST(ComponentValuesJsonTest, WrittenValues_RefuseAValueUnderTheWrongName)
{
    ComponentValues componentValues;

    componentValues.insert_or_assign(
        "component::Inventory", antwika::component::Health{});

    EXPECT_THROW(
        { std::ignore = getWrittenComponentValues(componentValues); }, LoadoutError);
}

TEST(ComponentValuesJsonTest, ReadValues_RefuseAStrangeName)
{
    nlohmann::json valuesJson;

    valuesJson["component::Stranger"] = nlohmann::json::object();

    EXPECT_THROW(
        { std::ignore = getReadComponentValues(valuesJson); }, LoadoutError);
}

TEST(ComponentValuesJsonTest, ReadValues_RefuseAStrangeField)
{
    nlohmann::json valuesJson;

    valuesJson["component::Health"]["mana"] = 7;

    EXPECT_THROW(
        { std::ignore = getReadComponentValues(valuesJson); }, LoadoutError);
}

TEST(ComponentValuesJsonTest, ReadValues_RefuseANonObjectDocument)
{
    EXPECT_THROW(
        { std::ignore = getReadComponentValues(nlohmann::json::array()); },
        LoadoutError);
}

TEST(ComponentValuesJsonTest, ReadValues_RefuseANonObjectEntry)
{
    nlohmann::json valuesJson;

    valuesJson["component::Health"] = 7;

    EXPECT_THROW(
        { std::ignore = getReadComponentValues(valuesJson); }, LoadoutError);
}

TEST(ComponentValuesJsonTest, ReadValues_FillMissingFieldsFromFresh)
{
    nlohmann::json valuesJson;

    valuesJson["component::Health"]["food"] = 12;

    const auto componentValues = getReadComponentValues(valuesJson);

    const auto &value = componentValues.at("component::Health");

    const auto expectedHealth =
        antwika::component::Health{.food = 12, .water = 240};

    EXPECT_TRUE(
        std::get<antwika::component::Health>(value) == expectedHealth);
}

TEST(ComponentValuesJsonTest, ReadValues_ClampWhatTheJsonOverstates)
{
    nlohmann::json valuesJson;

    valuesJson["component::Health"]["food"] = 999;

    const auto componentValues = getReadComponentValues(valuesJson);

    const auto expectedHealth = antwika::component::Health{.food = 240};

    EXPECT_TRUE(
        std::get<antwika::component::Health>(
            componentValues.at("component::Health"))
        == expectedHealth);
}
