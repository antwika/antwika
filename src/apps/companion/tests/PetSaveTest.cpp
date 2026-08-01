#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

#include <antwika/replay/SchemaVersion.hpp>

#include "antwika/companion/CompanionMemory.hpp"
#include "antwika/companion/Pet.hpp"
#include "antwika/companion/PetSave.hpp"
#include "antwika/companion/SaveFormatError.hpp"

using antwika::companion::CompanionMemory;
using antwika::companion::companionMemoryFromJson;
using antwika::companion::companionMemoryToJson;
using antwika::companion::energyCeilingFor;
using antwika::companion::kSaveFormatVersion;
using antwika::companion::kSaveMagic;
using antwika::companion::LineageMemory;
using antwika::companion::PetConfig;
using antwika::companion::PetMemory;
using antwika::companion::PetState;
using antwika::companion::readCompanionMemory;
using antwika::companion::SaveFormatError;
using antwika::companion::Saying;
using antwika::companion::standardPetMigrations;
using antwika::companion::writeCompanionMemory;

namespace
{
    CompanionMemory lived()
    {
        return CompanionMemory{
            .pet =
                PetMemory{
                    .ticks = 617,
                    .state = PetState::Asleep,
                    .saying = Saying::Zzz,
                    .sayingTicksLeft = 12,
                    .hunger = 3,
                    .fun = 5,
                    .happiness = 7,
                    .energy = 18,
                    .day = 4,
                    .meals = 4,
                    .plays = 6,
                    .disturbances = 1,
                    .pesters = 2,
                    .collapses = 1,
                    .woken = true},
            .lineage = LineageMemory{.generation = 3, .bestTicks = 900}};
    }

    // A document written by the build before energy was life.
    // Hand-written rather than produced by this build.
    // The whole point of a migration is that no build writes its input.
    nlohmann::json versionOne()
    {
        return nlohmann::json{
            {"magic", std::string(kSaveMagic)},
            {std::string(antwika::replay::kSchemaVersionKey), 1},
            {"ticks", 617},
            {"state", "asleep"},
            {"saying", "zzz"},
            {"sayingTicksLeft", 12},
            {"hunger", 3},
            {"happiness", 7},
            {"meals", 4},
            {"disturbances", 1},
            {"pesters", 2},
            {"disturbed", true}};
    }

    // A document from after energy was life.
    // And from before the file began keeping a record.
    nlohmann::json versionTwo()
    {
        return nlohmann::json{
            {"magic", std::string(kSaveMagic)},
            {std::string(antwika::replay::kSchemaVersionKey), 2},
            {"ticks", 200},
            {"state", "awake"},
            {"saying", "none"},
            {"sayingTicksLeft", 0},
            {"hunger", 2},
            {"fun", 6},
            {"happiness", 8},
            {"energy", 21},
            {"day", 2},
            {"meals", 3},
            {"plays", 4},
            {"disturbances", 0},
            {"pesters", 1},
            {"collapses", 0},
            {"woken", false}};
    }
} // namespace

TEST(PetSaveTest, ToJson_RoundTripsEveryFieldOfALivedCompanion)
{
    const CompanionMemory original = lived();

    EXPECT_EQ(
        companionMemoryFromJson(companionMemoryToJson(original)),
        original);
}

TEST(PetSaveTest, ToJson_RoundTripsAPerishedCompanion)
{
    CompanionMemory gone = lived();
    gone.pet.state = PetState::Perished;
    gone.pet.saying = Saying::None;
    gone.pet.sayingTicksLeft = 0;
    gone.pet.energy = 0;
    gone.pet.collapses = 9;

    EXPECT_EQ(
        companionMemoryFromJson(companionMemoryToJson(gone)), gone);
}

TEST(PetSaveTest, ToJson_StatesItsMagicAndItsVersion)
{
    const auto encoded = companionMemoryToJson(lived());

    EXPECT_EQ(encoded.at("magic"), std::string(kSaveMagic));
    EXPECT_EQ(
        encoded.at(std::string(antwika::replay::kSchemaVersionKey)),
        kSaveFormatVersion);
}

// Every name in the file is symbolic rather than a number.
// So reordering either enumeration cannot change what a file means.
TEST(PetSaveTest, ToJson_NamesTheStateAndTheSayingInWords)
{
    const auto encoded = companionMemoryToJson(lived());

    EXPECT_EQ(encoded.at("state"), "asleep");
    EXPECT_EQ(encoded.at("saying"), "zzz");
}

TEST(PetSaveTest, FromJson_RefusesADocumentThatIsNotAnObject)
{
    EXPECT_THROW(
        (void)companionMemoryFromJson(nlohmann::json::array()),
        SaveFormatError);
}

TEST(PetSaveTest, FromJson_RefusesAnotherFormatsMagic)
{
    auto encoded = companionMemoryToJson(lived());
    encoded["magic"] = "antwika-game-save";

    EXPECT_THROW(
        (void)companionMemoryFromJson(encoded), SaveFormatError);
}

TEST(PetSaveTest, FromJson_RefusesAMemberThatIsNotThere)
{
    auto encoded = companionMemoryToJson(lived());
    encoded.erase("happiness");

    EXPECT_THROW(
        (void)companionMemoryFromJson(encoded), SaveFormatError);
}

TEST(PetSaveTest, FromJson_RefusesAMemberOfTheWrongShape)
{
    auto encoded = companionMemoryToJson(lived());
    encoded["woken"] = "yes";

    EXPECT_THROW(
        (void)companionMemoryFromJson(encoded), SaveFormatError);
}

TEST(PetSaveTest, FromJson_RefusesAMemberThisFormatDoesNotHave)
{
    auto encoded = companionMemoryToJson(lived());
    encoded["favouriteColour"] = "blue";

    EXPECT_THROW(
        (void)companionMemoryFromJson(encoded), SaveFormatError);
}

TEST(PetSaveTest, FromJson_RefusesAStateThatIsNotOneOfTheThree)
{
    auto encoded = companionMemoryToJson(lived());
    encoded["state"] = "dreaming";

    EXPECT_THROW(
        (void)companionMemoryFromJson(encoded), SaveFormatError);
}

TEST(PetSaveTest, FromJson_RefusesALineThisBuildDoesNotHave)
{
    auto encoded = companionMemoryToJson(lived());
    encoded["saying"] = "goodMorning";

    EXPECT_THROW(
        (void)companionMemoryFromJson(encoded), SaveFormatError);
}

// The reader migrates before it validates.
// So a version this build cannot reach is refused rather than read.
// Satisfying today's schema by chance is not enough.
TEST(PetSaveTest, FromJson_RefusesAVersionFromANewerBuild)
{
    auto encoded = companionMemoryToJson(lived());
    encoded[std::string(antwika::replay::kSchemaVersionKey)] =
        kSaveFormatVersion + 1;

    EXPECT_THROW(
        (void)companionMemoryFromJson(encoded), SaveFormatError);
}

// What a version 1 companion becomes.
// What still means the same thing is carried across.
// The needs with no reading here start where a new companion's do.
TEST(PetSaveTest, FromJson_MigratesAVersionOneCompanion)
{
    const CompanionMemory carried =
        companionMemoryFromJson(versionOne());

    EXPECT_EQ(carried.pet.ticks, 617U);
    EXPECT_EQ(carried.pet.state, PetState::Asleep);
    EXPECT_EQ(carried.pet.saying, Saying::Zzz);
    EXPECT_EQ(carried.pet.hunger, 3U);
    EXPECT_EQ(carried.pet.happiness, 7U);
    EXPECT_EQ(carried.pet.meals, 4U);
    EXPECT_EQ(carried.pet.disturbances, 1U);
    EXPECT_EQ(carried.pet.pesters, 2U);

    // The needs version 1 never had.
    EXPECT_EQ(carried.pet.fun, antwika::companion::kFunStart);
    EXPECT_EQ(carried.pet.energy, antwika::companion::kEnergyBase);
    EXPECT_EQ(carried.pet.plays, 0U);
    EXPECT_EQ(carried.pet.collapses, 0U);
    EXPECT_EQ(carried.pet.day, 0U);

    // And the note that changed its name rather than its meaning.
    EXPECT_TRUE(carried.pet.woken);

    // The record a version 1 file never kept.
    EXPECT_EQ(carried.lineage.generation, 1U);
    EXPECT_EQ(carried.lineage.bestTicks, 0U);
}

// Version 1 said perished exactly when its happiness ran out.
// Here that is the ceiling running out, so a grave stays a grave.
TEST(PetSaveTest, FromJson_MigratesAVersionOnePerishedCompanionAsGone)
{
    auto document = versionOne();
    document["state"] = "perished";
    document["saying"] = "none";
    document["sayingTicksLeft"] = 0;
    document["happiness"] = 0;

    const CompanionMemory carried = companionMemoryFromJson(document);

    EXPECT_EQ(carried.pet.state, PetState::Perished);
    EXPECT_EQ(carried.pet.energy, 0U);
    EXPECT_EQ(
        energyCeilingFor(
            PetConfig{}, carried.pet.ticks, carried.pet.collapses),
        0U);
}

TEST(PetSaveTest, FromJson_MigratesAVersionTwoCompanion)
{
    const CompanionMemory carried =
        companionMemoryFromJson(versionTwo());

    EXPECT_EQ(carried.pet.ticks, 200U);
    EXPECT_EQ(carried.pet.energy, 21U);
    EXPECT_EQ(carried.pet.fun, 6U);
    EXPECT_EQ(carried.pet.plays, 4U);
    EXPECT_EQ(carried.lineage.generation, 1U);
    EXPECT_EQ(carried.lineage.bestTicks, 0U);
}

// A document with no version member is version 1.
TEST(PetSaveTest, FromJson_ReadsADocumentThatStatesNoVersionAsTheFirst)
{
    auto document = versionOne();
    document.erase(std::string(antwika::replay::kSchemaVersionKey));

    EXPECT_EQ(
        companionMemoryFromJson(document),
        companionMemoryFromJson(versionOne()));
}

TEST(PetSaveTest, StandardPetMigrations_StampsTheCurrentVersion)
{
    EXPECT_EQ(
        standardPetMigrations().currentVersion(), kSaveFormatVersion);
}

TEST(PetSaveTest, WriteCompanionMemory_RoundTripsThroughAStream)
{
    std::stringstream stream;
    writeCompanionMemory(lived(), stream);

    EXPECT_EQ(readCompanionMemory(stream), lived());
}

// Indented rather than compact, unlike a recorded replay.
// A companion is a couple of dozen lines somebody may want to read.
TEST(PetSaveTest, WriteCompanionMemory_WritesMoreThanOneLine)
{
    std::stringstream stream;
    writeCompanionMemory(lived(), stream);

    EXPECT_NE(stream.str().find('\n'), std::string::npos);
}

TEST(PetSaveTest, ReadCompanionMemory_RefusesAStreamThatIsNotJson)
{
    std::stringstream stream("this is not a companion");

    EXPECT_THROW((void)readCompanionMemory(stream), SaveFormatError);
}

// The comparison is field by field, and every field of it matters.
// A test that only ever compares whole memories proves half of it.
TEST(PetSaveTest, CompanionMemory_DiffersWheneverAnyOneFieldDoes)
{
    const CompanionMemory original = lived();

    CompanionMemory other = original;
    other.pet.ticks = 1;
    EXPECT_NE(other, original);

    other = original;
    other.pet.state = PetState::Awake;
    EXPECT_NE(other, original);

    other = original;
    other.pet.saying = Saying::Hello;
    EXPECT_NE(other, original);

    other = original;
    other.pet.sayingTicksLeft = 1;
    EXPECT_NE(other, original);

    other = original;
    other.pet.hunger = 1;
    EXPECT_NE(other, original);

    other = original;
    other.pet.fun = 1;
    EXPECT_NE(other, original);

    other = original;
    other.pet.happiness = 1;
    EXPECT_NE(other, original);

    other = original;
    other.pet.energy = 1;
    EXPECT_NE(other, original);

    other = original;
    other.pet.day = 1;
    EXPECT_NE(other, original);

    other = original;
    other.pet.meals = 99;
    EXPECT_NE(other, original);

    other = original;
    other.pet.plays = 99;
    EXPECT_NE(other, original);

    other = original;
    other.pet.disturbances = 99;
    EXPECT_NE(other, original);

    other = original;
    other.pet.pesters = 99;
    EXPECT_NE(other, original);

    other = original;
    other.pet.collapses = 99;
    EXPECT_NE(other, original);

    other = original;
    other.pet.woken = false;
    EXPECT_NE(other, original);

    other = original;
    other.lineage.generation = 99;
    EXPECT_NE(other, original);

    other = original;
    other.lineage.bestTicks = 99;
    EXPECT_NE(other, original);

    EXPECT_EQ(original, lived());
}
