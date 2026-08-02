#include <antwika/replay/MigrationChain.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <antwika/replay/IMigration.hpp>
#include <antwika/replay/ReplayMigrations.hpp>
#include <antwika/replay/SchemaVersion.hpp>
#include <antwika/replay/SchemaVersionError.hpp>

using antwika::replay::IMigration;
using antwika::replay::kReplayDocumentVersion;
using antwika::replay::MigrationChain;
using antwika::replay::MigrationList;
using antwika::replay::SchemaVersionError;
using antwika::replay::standardReplayMigrations;

namespace
{
    // The example migration the mechanism is proven with.
    // It appends its name to a "trail" array.
    // So a test can assert which steps ran, and in which order.
    // No production migration exists yet.
    // The format is still at version 1, so there is nothing to write.
    // Inventing a fake format change in src/ would be worse.
    class TrailMigration final : public IMigration
    {
    public:
        TrailMigration(
            std::uint32_t from, std::uint32_t to, std::string label)
            : from(from), to(to), label(std::move(label))
        {
        }

        [[nodiscard]] std::uint32_t fromVersion() const noexcept override
        {
            return from;
        }

        [[nodiscard]] std::uint32_t toVersion() const noexcept override
        {
            return to;
        }

        [[nodiscard]] std::string_view name() const noexcept override
        {
            return label;
        }

        void apply(nlohmann::json &document) const override
        {
            document["trail"].push_back(label);
        }

    private:
        std::uint32_t from;
        std::uint32_t to;
        std::string label;
    };

    std::shared_ptr<const IMigration> step(
        std::uint32_t from, std::uint32_t to, std::string label)
    {
        return std::make_shared<const TrailMigration>(
            from, to, std::move(label));
    }

    MigrationChain chainToThree()
    {
        MigrationList migrations;
        migrations.push_back(step(1, 2, "one-to-two"));
        migrations.push_back(step(2, 3, "two-to-three"));
        return MigrationChain(std::move(migrations), 3);
    }
} // namespace

TEST(MigrationChainTest, MigratesOneToTwoToThreeInOrder)
{
    nlohmann::json document;
    document["version"] = 1;

    chainToThree().migrate(document);

    EXPECT_EQ(document["version"], 3);
    EXPECT_EQ(
        document["trail"],
        nlohmann::json({"one-to-two", "two-to-three"}));
}

TEST(MigrationChainTest, StartsFromWhicheverVersionTheDocumentStates)
{
    nlohmann::json document;
    document["version"] = 2;

    chainToThree().migrate(document);

    EXPECT_EQ(document["version"], 3);
    EXPECT_EQ(document["trail"], nlohmann::json({"two-to-three"}));
}

TEST(MigrationChainTest, ADocumentWithNoVersionIsMigratedFromOne)
{
    nlohmann::json document = nlohmann::json::object();

    chainToThree().migrate(document);

    EXPECT_EQ(document["version"], 3);
    EXPECT_EQ(
        document["trail"],
        nlohmann::json({"one-to-two", "two-to-three"}));
}

TEST(MigrationChainTest, ADocumentAlreadyCurrentIsUntouched)
{
    nlohmann::json document;
    document["version"] = 3;
    document["kept"] = "as it was";

    chainToThree().migrate(document);

    EXPECT_EQ(document["version"], 3);
    EXPECT_EQ(document["kept"], "as it was");
    EXPECT_FALSE(document.contains("trail"));
}

TEST(MigrationChainTest, AGapInTheChainIsReportedWithBothVersions)
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

TEST(MigrationChainTest, ADocumentNewerThanCurrentIsRefused)
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

// Version 0 predates every migration this chain carries.
// It used to load silently when there was nothing to migrate, and
// to be blamed on the build's own chain when there was.
TEST(MigrationChainTest, AVersionOlderThanEveryMigrationIsRefused)
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

TEST(MigrationChainTest, AnUnreadableVersionIsRefused)
{
    nlohmann::json document;
    document["version"] = "two";

    EXPECT_THROW(chainToThree().migrate(document), SchemaVersionError);
}

TEST(MigrationChainTest, ADocumentThatIsNotAnObjectIsLeftAlone)
{
    nlohmann::json document = nlohmann::json::array({1, 2});

    chainToThree().migrate(document);

    EXPECT_EQ(document, nlohmann::json::array({1, 2}));
}

TEST(MigrationChainTest, AMigrationThatIsNotASingleStepIsRefused)
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

TEST(MigrationChainTest, TwoMigrationsReadingOneVersionAreRefused)
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

TEST(MigrationChainTest, ACallerMayNameItsOwnVersionKey)
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

TEST(MigrationChainTest, TheStandardReplayChainTargetsTheCurrentVersion)
{
    const MigrationChain chain = standardReplayMigrations();
    EXPECT_EQ(chain.currentVersion(), kReplayDocumentVersion);

    nlohmann::json document;
    document["version"] = kReplayDocumentVersion;
    chain.migrate(document);
    EXPECT_EQ(document["version"], kReplayDocumentVersion);
}
