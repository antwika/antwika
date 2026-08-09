#include <gtest/gtest.h>

#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unistd.h>

#include <antwika/testing/ScratchPath.hpp>

#include "antwika/companion/CompanionMemory.hpp"
#include "antwika/companion/FilePetStore.hpp"
#include "antwika/companion/Pet.hpp"
#include "antwika/companion/SaveFormatError.hpp"

using antwika::companion::CompanionMemory;
using antwika::companion::FilePetStore;
using antwika::companion::LineageMemory;
using antwika::companion::PetMemory;
using antwika::companion::PetState;
using antwika::companion::SaveFormatError;
using antwika::companion::Saying;

namespace
{
    CompanionMemory lived()
    {
        return CompanionMemory{
            .pet =
                PetMemory{
                    .ticks = 240,
                    .state = PetState::Awake,
                    .saying = Saying::FeedMe,
                    .sayingTicksLeft = 40,
                    .hunger = 5,
                    .fun = 4,
                    .happiness = 6,
                    .energy = 22,
                    .day = 2,
                    .meals = 2,
                    .plays = 3,
                    .disturbances = 0,
                    .pesters = 1,
                    .collapses = 0,
                    .woken = false},
            .lineage = LineageMemory{.generation = 2, .bestTicks = 500}};
    }
}

TEST(FilePetStoreTest, Load_AnswersNothingWhenThereIsNoPreviousCompanion)
{
    const antwika::testing::ScratchFile file("antwika_companion_absent.json");
    FilePetStore store(file.string());

    EXPECT_FALSE(store.load().has_value());
}

TEST(FilePetStoreTest, Save_RoundTripsThroughTheFile)
{
    const antwika::testing::ScratchFile file(
        "antwika_companion_round_trip.json");
    FilePetStore store(file.string());

    store.save(lived());

    const auto loaded = store.load();
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(*loaded, lived());
}

TEST(FilePetStoreTest, Load_RefusesAFileThatIsNotACompanion)
{
    const antwika::testing::ScratchFile file(
        "antwika_companion_malformed.json");
    {
        std::ofstream out(file.string());
        out << "{ \"magic\": ";
    }

    FilePetStore store(file.string());

    EXPECT_THROW((void)store.load(), SaveFormatError);
}

TEST(FilePetStoreTest, Save_RefusesAPathThatCannotBeWritten)
{
    FilePetStore store("/nonexistent-directory/companion.json");

    EXPECT_THROW(store.save(lived()), SaveFormatError);
}
