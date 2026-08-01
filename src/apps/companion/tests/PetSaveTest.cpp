#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

#include <antwika/replay/SchemaVersion.hpp>

#include "antwika/companion/Pet.hpp"
#include "antwika/companion/PetSave.hpp"
#include "antwika/companion/SaveFormatError.hpp"

using antwika::companion::kSaveFormatVersion;
using antwika::companion::kSaveMagic;
using antwika::companion::PetMemory;
using antwika::companion::petMemoryFromJson;
using antwika::companion::petMemoryToJson;
using antwika::companion::PetState;
using antwika::companion::readPetMemory;
using antwika::companion::SaveFormatError;
using antwika::companion::Saying;
using antwika::companion::standardPetMigrations;
using antwika::companion::writePetMemory;

namespace
{
    PetMemory lived()
    {
        return PetMemory{
            .ticks = 617,
            .state = PetState::Asleep,
            .saying = Saying::Zzz,
            .sayingTicksLeft = 12,
            .hunger = 3,
            .happiness = 7,
            .meals = 4,
            .disturbances = 1,
            .pesters = 2,
            .disturbed = true};
    }
} // namespace

TEST(PetSaveTest, ToJson_RoundTripsEveryFieldOfALivedCompanion)
{
    const PetMemory original = lived();

    EXPECT_EQ(petMemoryFromJson(petMemoryToJson(original)), original);
}

TEST(PetSaveTest, ToJson_RoundTripsAPerishedCompanion)
{
    PetMemory gone = lived();
    gone.state = PetState::Perished;
    gone.saying = Saying::None;
    gone.happiness = 0;

    EXPECT_EQ(petMemoryFromJson(petMemoryToJson(gone)), gone);
}

TEST(PetSaveTest, ToJson_StatesItsMagicAndItsVersion)
{
    const auto encoded = petMemoryToJson(lived());

    EXPECT_EQ(encoded.at("magic"), std::string(kSaveMagic));
    EXPECT_EQ(
        encoded.at(std::string(antwika::replay::kSchemaVersionKey)),
        kSaveFormatVersion);
}

// Every name in the file is symbolic rather than a number.
// So reordering either enumeration cannot change what a file means.
TEST(PetSaveTest, ToJson_NamesTheStateAndTheSayingInWords)
{
    const auto encoded = petMemoryToJson(lived());

    EXPECT_EQ(encoded.at("state"), "asleep");
    EXPECT_EQ(encoded.at("saying"), "zzz");
}

TEST(PetSaveTest, FromJson_RefusesADocumentThatIsNotAnObject)
{
    EXPECT_THROW(
        (void)petMemoryFromJson(nlohmann::json::array()),
        SaveFormatError);
}

TEST(PetSaveTest, FromJson_RefusesAnotherFormatsMagic)
{
    auto encoded = petMemoryToJson(lived());
    encoded["magic"] = "antwika-game-save";

    EXPECT_THROW((void)petMemoryFromJson(encoded), SaveFormatError);
}

TEST(PetSaveTest, FromJson_RefusesAMemberThatIsNotThere)
{
    auto encoded = petMemoryToJson(lived());
    encoded.erase("happiness");

    EXPECT_THROW((void)petMemoryFromJson(encoded), SaveFormatError);
}

TEST(PetSaveTest, FromJson_RefusesAMemberOfTheWrongShape)
{
    auto encoded = petMemoryToJson(lived());
    encoded["disturbed"] = "yes";

    EXPECT_THROW((void)petMemoryFromJson(encoded), SaveFormatError);
}

TEST(PetSaveTest, FromJson_RefusesAMemberThisFormatDoesNotHave)
{
    auto encoded = petMemoryToJson(lived());
    encoded["favouriteColour"] = "blue";

    EXPECT_THROW((void)petMemoryFromJson(encoded), SaveFormatError);
}

TEST(PetSaveTest, FromJson_RefusesAStateThatIsNotOneOfTheThree)
{
    auto encoded = petMemoryToJson(lived());
    encoded["state"] = "dreaming";

    EXPECT_THROW((void)petMemoryFromJson(encoded), SaveFormatError);
}

TEST(PetSaveTest, FromJson_RefusesALineThisBuildDoesNotHave)
{
    auto encoded = petMemoryToJson(lived());
    encoded["saying"] = "goodMorning";

    EXPECT_THROW((void)petMemoryFromJson(encoded), SaveFormatError);
}

// The reader migrates before it validates.
// So a version this build cannot reach is refused rather than read.
// Satisfying today's schema by chance is not enough.
TEST(PetSaveTest, FromJson_RefusesAVersionFromANewerBuild)
{
    auto encoded = petMemoryToJson(lived());
    encoded[std::string(antwika::replay::kSchemaVersionKey)] =
        kSaveFormatVersion + 1;

    EXPECT_THROW((void)petMemoryFromJson(encoded), SaveFormatError);
}

// A document with no version member is version 1.
// Which is what this format writes, so one still reads.
TEST(PetSaveTest, FromJson_ReadsADocumentThatStatesNoVersionAsTheFirst)
{
    auto encoded = petMemoryToJson(lived());
    encoded.erase(std::string(antwika::replay::kSchemaVersionKey));

    EXPECT_EQ(petMemoryFromJson(encoded), lived());
}

TEST(PetSaveTest, StandardPetMigrations_StampsTheCurrentVersion)
{
    EXPECT_EQ(
        standardPetMigrations().currentVersion(), kSaveFormatVersion);
}

TEST(PetSaveTest, WritePetMemory_RoundTripsThroughAStream)
{
    std::stringstream stream;
    writePetMemory(lived(), stream);

    EXPECT_EQ(readPetMemory(stream), lived());
}

// Indented rather than compact, unlike a recorded replay.
// A companion is a dozen lines somebody may want to read.
TEST(PetSaveTest, WritePetMemory_WritesMoreThanOneLine)
{
    std::stringstream stream;
    writePetMemory(lived(), stream);

    EXPECT_NE(stream.str().find('\n'), std::string::npos);
}

TEST(PetSaveTest, ReadPetMemory_RefusesAStreamThatIsNotJson)
{
    std::stringstream stream("this is not a companion");

    EXPECT_THROW((void)readPetMemory(stream), SaveFormatError);
}

// The comparison is field by field, and every field of it matters.
// A test that only ever compares whole memories proves half of it.
TEST(PetSaveTest, PetMemory_DiffersWheneverAnyOneFieldDoes)
{
    const PetMemory original = lived();

    PetMemory other = original;
    other.ticks = 1;
    EXPECT_NE(other, original);

    other = original;
    other.state = PetState::Awake;
    EXPECT_NE(other, original);

    other = original;
    other.saying = Saying::Hello;
    EXPECT_NE(other, original);

    other = original;
    other.sayingTicksLeft = 1;
    EXPECT_NE(other, original);

    other = original;
    other.hunger = 1;
    EXPECT_NE(other, original);

    other = original;
    other.happiness = 1;
    EXPECT_NE(other, original);

    other = original;
    other.meals = 99;
    EXPECT_NE(other, original);

    other = original;
    other.disturbances = 99;
    EXPECT_NE(other, original);

    other = original;
    other.pesters = 99;
    EXPECT_NE(other, original);

    other = original;
    other.disturbed = false;
    EXPECT_NE(other, original);

    EXPECT_EQ(original, lived());
}
