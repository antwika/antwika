#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <antwika/schema/Step.hpp>

using antwika::schema::getMigration;

TEST(StepTest, Step_KeepsTheVersionsItWasGiven)
{
    const auto madeStep = getMigration(3, 4, "three-to-four", [](nlohmann::json &) {});

    EXPECT_EQ(madeStep->getFromVersion(), 3U);
    EXPECT_EQ(madeStep->toVersion(), 4U);
}

TEST(StepTest, Step_KeepsTheNameItWasGiven)
{
    const auto madeStep = getMigration(3, 4, "three-to-four", [](nlohmann::json &) {});

    EXPECT_EQ(madeStep->getName(), "three-to-four");
}

TEST(StepTest, Apply_RunsWhatTheCallerGave)
{
    const auto madeStep = getMigration(
        1, 2, "one-to-two", [](nlohmann::json &document) {
            document["laid"] = true;
        });

    nlohmann::json document;
    madeStep->apply(document);

    EXPECT_EQ(document["laid"], true);
}
