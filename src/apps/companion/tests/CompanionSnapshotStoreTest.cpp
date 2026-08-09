#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <antwika/console/SnapshotError.hpp>
#include <antwika/console/SnapshotFormat.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include "antwika/companion/CompanionSnapshotStore.hpp"
#include "antwika/companion/Lineage.hpp"
#include "antwika/companion/Pet.hpp"

using antwika::companion::CompanionSnapshotStore;
using antwika::companion::Lineage;
using antwika::companion::Pet;
using antwika::companion::PetConfig;
using antwika::console::SnapshotError;

namespace
{
    antwika::console::SnapshotFormat dumpFormat()
    {
        return antwika::console::SnapshotFormat(
            {.magic = antwika::companion::kStateDumpMagic,
             .version = antwika::companion::kStateDumpVersion},
            "antwika companion state dump document",
            antwika::companion::standardStateDumpMigrations);
    }
}

TEST(CompanionSnapshotStoreTest, Load_ComesBackToWhatDumpWrote)
{
    const antwika::testing::ScratchFile file(
        "antwika_companion_store_round_trip.json");
    const auto path = file.path().string();

    const PetConfig config{};
    Pet lived(config);
    Lineage lineage;
    lived.pester();
    lived.step();
    lineage.record(lived.ticks());
    CompanionSnapshotStore store(lived, lineage);

    store.dump(path, {"> dump_state", "dumped"});

    Pet fresh(config);
    Lineage freshLineage;
    CompanionSnapshotStore loaded(fresh, freshLineage);

    const auto history = loaded.load(path);

    EXPECT_EQ(
        history,
        (std::vector<std::string>{"> dump_state", "dumped"}));
    EXPECT_EQ(fresh.remember(), lived.remember());
    EXPECT_EQ(freshLineage.remember(), lineage.remember());
}

TEST(CompanionSnapshotStoreTest, Load_RewrapsAStateThatWillNotRead)
{
    const antwika::testing::ScratchFile file(
        "antwika_companion_store_bad_state.json");
    const auto path = file.path().string();

    dumpFormat().write(
        antwika::console::Snapshot{.console = {}}, path);

    Pet pet{PetConfig{}};
    Lineage lineage;
    CompanionSnapshotStore store(pet, lineage);

    EXPECT_THROW((void)store.load(path), SnapshotError);
}

TEST(CompanionSnapshotStoreTest, Load_RefusesAnotherApplicationsDump)
{
    const antwika::testing::ScratchFile file(
        "antwika_companion_store_wrong_magic.json");
    const auto path = file.path().string();

    const antwika::console::SnapshotFormat other(
        {.magic = "antwika-game-state-dump",
         .version = 1},
        "antwika game state dump document",
        antwika::companion::standardStateDumpMigrations);
    other.write(antwika::console::Snapshot{.console = {}}, path);

    Pet pet{PetConfig{}};
    Lineage lineage;
    CompanionSnapshotStore store(pet, lineage);

    EXPECT_THROW((void)store.load(path), SnapshotError);
}
