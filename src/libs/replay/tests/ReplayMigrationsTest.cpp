#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <antwika/schema/MigrationChain.hpp>
#include <antwika/replay/ReplayMigrations.hpp>
#include <antwika/replay/ReplayVersions.hpp>

using antwika::schema::MigrationChain;
using antwika::replay::kReplayDocumentVersion;
using antwika::replay::getStandardReplayMigrations;

TEST(ReplayMigrationsTest, StandardReplayMigrations_TargetCurrent)
{
    const MigrationChain chain = getStandardReplayMigrations();
    EXPECT_EQ(chain.getCurrentVersion(), kReplayDocumentVersion);

    nlohmann::json document;
    document["version"] = kReplayDocumentVersion;
    chain.migrate(document);
    EXPECT_EQ(document["version"], kReplayDocumentVersion);
}
