#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

#include <antwika/input/Key.hpp>
#include <antwika/replay/SchemaVersion.hpp>

#include "ScratchDirectory.hpp"
#include "antwika/game/Action.hpp"
#include "antwika/game/KeyBindings.hpp"
#include "antwika/game/OptionsFile.hpp"
#include "antwika/game/OptionsFormatError.hpp"

using antwika::game::tests::scratchDirectory;

using antwika::game::Action;
using antwika::game::bindingsFromJson;
using antwika::game::bindingsToJson;
using antwika::game::BindOutcome;
using antwika::game::kDefaultBindings;
using antwika::game::KeyBindings;
using antwika::game::kOptionsFormatVersion;
using antwika::game::kOptionsMagic;
using antwika::game::loadOptionsFileOrDefaults;
using antwika::game::machineOptionsFor;
using antwika::game::OptionsFormatError;
using antwika::game::readOptions;
using antwika::game::saveOptionsFile;
using antwika::game::saveOptionsFileIfNamed;
using antwika::game::standardOptionsMigrations;
using antwika::game::writeOptions;
using antwika::input::Key;

namespace
{
    [[nodiscard]] KeyBindings rebound()
    {
        KeyBindings bindings;
        EXPECT_EQ(bindings.bind(Action::Pause, Key::J), BindOutcome::Bound);
        EXPECT_EQ(
            bindings.bind(Action::ResetView, Key::K), BindOutcome::Bound);
        return bindings;
    }

    [[nodiscard]] std::string versionKey()
    {
        return std::string(antwika::replay::kSchemaVersionKey);
    }

    class OptionsFileTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            std::filesystem::create_directories(directory);
        }

        void TearDown() override
        {
            std::error_code ignored;
            std::filesystem::remove_all(directory, ignored);
        }

        [[nodiscard]] std::string pathIn(const std::string &name) const
        {
            return (directory / name).string();
        }

        void writeText(const std::string &name, const std::string &text)
        {
            std::ofstream file(pathIn(name));
            file << text;
        }

        std::filesystem::path directory{scratchDirectory("options.")};
    };
} // namespace

TEST_F(OptionsFileTest, ADocumentStatesItsFormatAndItsVersion)
{
    const auto encoded = bindingsToJson(kDefaultBindings);

    EXPECT_EQ(encoded.at("magic").get<std::string>(), kOptionsMagic);
    EXPECT_EQ(
        encoded.at(versionKey()).get<std::uint32_t>(),
        kOptionsFormatVersion);
}

TEST_F(OptionsFileTest, ALayoutRoundTripsThroughTheDocument)
{
    EXPECT_EQ(bindingsFromJson(bindingsToJson(rebound())), rebound());
    EXPECT_EQ(
        bindingsFromJson(bindingsToJson(kDefaultBindings)),
        kDefaultBindings);
}

TEST_F(OptionsFileTest, ALayoutRoundTripsThroughAStream)
{
    std::stringstream stream;
    writeOptions(rebound(), stream);

    EXPECT_EQ(readOptions(stream), rebound());
}

TEST_F(OptionsFileTest, ALayoutRoundTripsThroughAFile)
{
    saveOptionsFile(rebound(), pathIn("options.json"));

    EXPECT_EQ(
        loadOptionsFileOrDefaults(pathIn("options.json")), rebound());
}

// A player who never opened this screen plays what it ships.
// That is a state, not a failure.
TEST_F(OptionsFileTest, AMissingFileIsAnOrdinaryFirstRun)
{
    EXPECT_EQ(
        loadOptionsFileOrDefaults(pathIn("nothing-here.json")),
        kDefaultBindings);
}

TEST_F(OptionsFileTest, TextThatIsNotJsonIsRefused)
{
    writeText("options.json", "not json at all");

    EXPECT_THROW(
        (void)loadOptionsFileOrDefaults(pathIn("options.json")),
        OptionsFormatError);
}

TEST_F(OptionsFileTest, ADocumentOfAnotherFormatIsRefused)
{
    auto document = bindingsToJson(kDefaultBindings);
    document["magic"] = "antwika-game-save";

    EXPECT_THROW((void)bindingsFromJson(document), OptionsFormatError);
}

// Read before anything is decoded.
// So a file from a build this one has never met is refused.
// Rather than read for happening to satisfy today's schema.
TEST_F(OptionsFileTest, ADocumentFromANewerBuildIsRefused)
{
    auto document = bindingsToJson(kDefaultBindings);
    document[versionKey()] = kOptionsFormatVersion + 1;

    EXPECT_THROW((void)bindingsFromJson(document), OptionsFormatError);
}

TEST_F(OptionsFileTest, ADocumentOfTheWrongShapeIsRefused)
{
    auto document = bindingsToJson(kDefaultBindings);
    document["bindings"][0].erase("key");

    EXPECT_THROW((void)bindingsFromJson(document), OptionsFormatError);
}

TEST_F(OptionsFileTest, ADocumentNamingAnActionThisBuildLacksIsRefused)
{
    auto document = bindingsToJson(kDefaultBindings);
    document["bindings"][0]["action"] = "fly";

    EXPECT_THROW((void)bindingsFromJson(document), OptionsFormatError);
}

TEST_F(OptionsFileTest, ADocumentNamingAKeyThisBuildLacksIsRefused)
{
    auto document = bindingsToJson(kDefaultBindings);
    document["bindings"][0]["key"] = "Joystick";

    EXPECT_THROW((void)bindingsFromJson(document), OptionsFormatError);
}

// Refused rather than repaired, exactly as a save's links are.
// A layout nobody could have chosen is not one to guess at.
TEST_F(OptionsFileTest, ADocumentBindingTwoActionsToOneKeyIsRefused)
{
    auto document = bindingsToJson(kDefaultBindings);
    document["bindings"][1]["key"] =
        document["bindings"][0]["key"];

    EXPECT_THROW((void)bindingsFromJson(document), OptionsFormatError);
}

TEST_F(OptionsFileTest, ADocumentBindingAReservedKeyIsRefused)
{
    auto document = bindingsToJson(kDefaultBindings);
    document["bindings"][0]["key"] = "Escape";

    EXPECT_THROW((void)bindingsFromJson(document), OptionsFormatError);
}

TEST_F(OptionsFileTest, AnUnwritablePathIsRefused)
{
    EXPECT_THROW(
        saveOptionsFile(kDefaultBindings, pathIn("no/such/dir/o.json")),
        OptionsFormatError);
}

// A full disk fails on the flush rather than on the open.
// Which is why the write is flushed here and not by a destructor.
TEST_F(OptionsFileTest, AStreamThatCannotTakeTheDocumentIsRefused)
{
    EXPECT_THROW(
        saveOptionsFile(kDefaultBindings, "/dev/full"),
        OptionsFormatError);
}

TEST_F(OptionsFileTest, NamingNowhereWritesNothing)
{
    saveOptionsFileIfNamed(rebound(), std::nullopt);

    saveOptionsFileIfNamed(rebound(), pathIn("options.json"));
    EXPECT_TRUE(std::filesystem::exists(pathIn("options.json")));
}

// There has only ever been one revision.
// The chain is here anyway.
// It is what refuses a document from a newer build.
TEST_F(OptionsFileTest, TheChainBringsDocumentsToTheCurrentVersion)
{
    EXPECT_EQ(
        standardOptionsMigrations().currentVersion(),
        kOptionsFormatVersion);
}

// **A replay reads nothing of the machine's and writes nothing to it.**
// Reading would resolve a key press against a layout nobody held.
// Writing would leave whoever replayed somebody else's session.
// Carrying that session's bindings.
TEST_F(OptionsFileTest, AReplayIsToldNothingAboutTheMachine)
{
    saveOptionsFile(rebound(), pathIn("options.json"));

    const auto machine = machineOptionsFor(true, pathIn("options.json"));

    EXPECT_FALSE(machine.bindings.has_value());
    EXPECT_FALSE(machine.path.has_value());
}

TEST_F(OptionsFileTest, ALiveRunIsToldWhatTheMachineHoldsAndWhereItIs)
{
    saveOptionsFile(rebound(), pathIn("options.json"));

    const auto machine = machineOptionsFor(false, pathIn("options.json"));

    EXPECT_EQ(machine.bindings, rebound());
    EXPECT_EQ(machine.path, pathIn("options.json"));
}

TEST_F(OptionsFileTest, ALiveRunWithNoFileYetIsToldTheShippedLayout)
{
    const auto machine =
        machineOptionsFor(false, pathIn("nothing-here.json"));

    EXPECT_EQ(machine.bindings, kDefaultBindings);
    EXPECT_EQ(machine.path, pathIn("nothing-here.json"));
}
