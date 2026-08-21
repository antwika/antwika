#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <antwika/schema/MigrationChain.hpp>
#include <antwika/replay/ReplayMigrations.hpp>
#include <antwika/replay/ReplayVersions.hpp>

using antwika::schema::MigrationChain;
using antwika::replay::kReplayDocumentVersion;
using antwika::replay::standardReplayMigrations;

TEST(ReplayMigrationsTest, StandardReplayMigrations_TargetCurrent)
{
    const MigrationChain chain = standardReplayMigrations();
    EXPECT_EQ(chain.currentVersion(), kReplayDocumentVersion);

    nlohmann::json document;
    document["version"] = kReplayDocumentVersion;
    chain.migrate(document);
    EXPECT_EQ(document["version"], kReplayDocumentVersion);
}
