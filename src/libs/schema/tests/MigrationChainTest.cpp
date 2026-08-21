#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include <antwika/schema/fakes/FakeTrailMigration.hpp>

#include <antwika/schema/MigrationChain.hpp>
#include <antwika/schema/IMigration.hpp>
#include <antwika/schema/SchemaVersion.hpp>
#include <antwika/schema/SchemaVersionError.hpp>

using antwika::schema::IMigration;
using antwika::schema::fakes::FakeTrailMigration;
using antwika::schema::MigrationChain;
using antwika::schema::MigrationList;
using antwika::schema::SchemaVersionError;

namespace
{
    std::shared_ptr<const IMigration> step(
        std::uint32_t fromVersion, std::uint32_t toVersion, std::string label)
    {
        return std::make_shared<const FakeTrailMigration>(
            fromVersion, toVersion, std::move(label));
    }

    MigrationChain chainToThree()
    {
        MigrationList migrations;
        migrations.push_back(step(1, 2, "one-to-two"));
        migrations.push_back(step(2, 3, "two-to-three"));
        return MigrationChain(std::move(migrations), 3);
    }
}

TEST(MigrationChainTest, Migrate_StepsThroughEveryVersionInOrder)
{
    nlohmann::json document;
    document["version"] = 1;

    chainToThree().migrate(document);

    EXPECT_EQ(document["version"], 3);
    EXPECT_EQ(
        document["trail"],
        nlohmann::json({"one-to-two", "two-to-three"}));
}

TEST(MigrationChainTest, Migrate_StartsFromTheStatedVersion)
{
    nlohmann::json document;
    document["version"] = 2;

    chainToThree().migrate(document);

    EXPECT_EQ(document["version"], 3);
    EXPECT_EQ(document["trail"], nlohmann::json({"two-to-three"}));
}

TEST(MigrationChainTest, Migrate_StartsFromOneWithNoVersion)
{
    nlohmann::json document = nlohmann::json::object();

    chainToThree().migrate(document);

    EXPECT_EQ(document["version"], 3);
    EXPECT_EQ(
        document["trail"],
        nlohmann::json({"one-to-two", "two-to-three"}));
}

TEST(MigrationChainTest, Migrate_LeavesACurrentDocumentAlone)
{
    nlohmann::json document;
    document["version"] = 3;
    document["kept"] = "as it was";

    chainToThree().migrate(document);

    EXPECT_EQ(document["version"], 3);
    EXPECT_EQ(document["kept"], "as it was");
    EXPECT_FALSE(document.contains("trail"));
}

TEST(MigrationChainTest, Migrate_ReportsAGapWithBothVersions)
{
    MigrationList migrations;
    migrations.push_back(step(1, 2, "one-to-two"));
    const MigrationChain chain(std::move(migrations), 3);

    nlohmann::json document;
    document["version"] = 1;
    try
    {
        chain.migrate(document);
        FAIL() << "expected a SchemaVersionError";
    }
    catch (const SchemaVersionError &error)
    {
        const std::string message = error.what();
        EXPECT_NE(message.find("gap"), std::string::npos);
        EXPECT_NE(
            message.find("no migration reads version 2"),
            std::string::npos);
    }
}

TEST(MigrationChainTest, Migrate_RefusesANewerDocument)
{
    nlohmann::json document;
    document["version"] = 4;

    try
    {
        chainToThree().migrate(document);
        FAIL() << "expected a SchemaVersionError";
    }
    catch (const SchemaVersionError &error)
    {
        const std::string message = error.what();
        EXPECT_NE(message.find("version 4"), std::string::npos);
        EXPECT_NE(
            message.find("up to version 3"), std::string::npos);
    }
}

TEST(MigrationChainTest, Migrate_RefusesATooOldVersion)
{
    nlohmann::json document;
    document["version"] = 0;

    try
    {
        chainToThree().migrate(document);
        FAIL() << "expected a SchemaVersionError";
    }
    catch (const SchemaVersionError &error)
    {
        const std::string message = error.what();
        EXPECT_NE(message.find("version 0"), std::string::npos);
        EXPECT_NE(
            message.find("versions 1 through 3"), std::string::npos);
        EXPECT_NE(
            message.find("no release ever wrote"), std::string::npos);
    }
}

TEST(MigrationChainTest, Migrate_RefusesAnUnreadableVersion)
{
    nlohmann::json document;
    document["version"] = "two";

    EXPECT_THROW(chainToThree().migrate(document), SchemaVersionError);
}

TEST(MigrationChainTest, Migrate_LeavesANonObjectAlone)
{
    nlohmann::json document = nlohmann::json::array({1, 2});

    chainToThree().migrate(document);

    EXPECT_EQ(document, nlohmann::json::array({1, 2}));
}

TEST(MigrationChainTest, Migrate_RefusesAMultiStepMigration)
{
    MigrationList migrations;
    migrations.push_back(step(1, 3, "one-to-three"));

    try
    {
        const MigrationChain chain(std::move(migrations), 3);
        FAIL() << "expected a SchemaVersionError";
    }
    catch (const SchemaVersionError &error)
    {
        const std::string message = error.what();
        EXPECT_NE(message.find("one-to-three"), std::string::npos);
        EXPECT_NE(message.find("single step"), std::string::npos);
    }
}

TEST(MigrationChainTest, Migrate_RefusesTwoStepsFromOneVersion)
{
    MigrationList migrations;
    migrations.push_back(step(1, 2, "one-to-two"));
    migrations.push_back(step(1, 2, "one-to-two-again"));

    try
    {
        const MigrationChain chain(std::move(migrations), 2);
        FAIL() << "expected a SchemaVersionError";
    }
    catch (const SchemaVersionError &error)
    {
        const std::string message = error.what();
        EXPECT_NE(message.find("one-to-two"), std::string::npos);
        EXPECT_NE(
            message.find("one-to-two-again"), std::string::npos);
        EXPECT_NE(message.find("shadowed"), std::string::npos);
    }
}

TEST(MigrationChainTest, Migrate_AcceptsACallersOwnVersionKey)
{
    MigrationList migrations;
    migrations.push_back(step(1, 2, "one-to-two"));
    const MigrationChain chain(
        std::move(migrations), 2, "schemaVersion");

    nlohmann::json document;
    document["schemaVersion"] = 1;
    chain.migrate(document);

    EXPECT_EQ(document["schemaVersion"], 2);
    EXPECT_EQ(document["trail"], nlohmann::json({"one-to-two"}));
}

