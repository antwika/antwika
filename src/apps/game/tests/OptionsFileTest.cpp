#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

#include <antwika/input/Key.hpp>
#include <antwika/replay/SchemaVersion.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include "antwika/game/Action.hpp"
#include <antwika/i18n/Locale.hpp>

#include "antwika/game/KeyBindings.hpp"
#include "antwika/game/OptionsFile.hpp"
#include "antwika/game/OptionsFormatError.hpp"


using antwika::game::Action;
using antwika::game::optionsFromJson;
using antwika::game::optionsToJson;
using antwika::game::PlayerOptions;
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
    [[nodiscard]] PlayerOptions rebound()
    {
        KeyBindings bindings;
        EXPECT_EQ(bindings.bind(Action::Pause, Key::J), BindOutcome::Bound);
        EXPECT_EQ(
            bindings.bind(Action::ResetView, Key::K), BindOutcome::Bound);

        // A language, a board and a key all away from their defaults.
        // So a round trip that dropped any one is a red test.
        return PlayerOptions{
            .bindings = bindings,
            .locale = antwika::i18n::Locale::Swedish,
            .keyboard = antwika::game::KeyboardLayout::English};
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

        std::filesystem::path directory{
            antwika::testing::scratchPath("options.")};
    };
} // namespace

TEST_F(OptionsFileTest, ADocumentStatesItsFormatAndItsVersion)
{
    const auto encoded = optionsToJson(PlayerOptions{});

    EXPECT_EQ(encoded.at("magic").get<std::string>(), kOptionsMagic);
    EXPECT_EQ(
        encoded.at(versionKey()).get<std::uint32_t>(),
        kOptionsFormatVersion);
}

TEST_F(OptionsFileTest, ALayoutRoundTripsThroughTheDocument)
{
    EXPECT_EQ(optionsFromJson(optionsToJson(rebound())), rebound());
    EXPECT_EQ(
        optionsFromJson(optionsToJson(PlayerOptions{})),
        PlayerOptions{});
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
        PlayerOptions{});
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
    auto document = optionsToJson(PlayerOptions{});
    document["magic"] = "antwika-game-save";

    EXPECT_THROW((void)optionsFromJson(document), OptionsFormatError);
}

// Read before anything is decoded.
// So a file from a build this one has never met is refused.
// Rather than read for happening to satisfy today's schema.
TEST_F(OptionsFileTest, ADocumentFromANewerBuildIsRefused)
{
    auto document = optionsToJson(PlayerOptions{});
    document[versionKey()] = kOptionsFormatVersion + 1;

    EXPECT_THROW((void)optionsFromJson(document), OptionsFormatError);
}

TEST_F(OptionsFileTest, ADocumentOfTheWrongShapeIsRefused)
{
    auto document = optionsToJson(PlayerOptions{});
    document["bindings"][0].erase("key");

    EXPECT_THROW((void)optionsFromJson(document), OptionsFormatError);
}

TEST_F(OptionsFileTest, ADocumentNamingAnActionThisBuildLacksIsRefused)
{
    auto document = optionsToJson(PlayerOptions{});
    document["bindings"][0]["action"] = "fly";

    EXPECT_THROW((void)optionsFromJson(document), OptionsFormatError);
}

TEST_F(OptionsFileTest, ADocumentNamingAKeyThisBuildLacksIsRefused)
{
    auto document = optionsToJson(PlayerOptions{});
    document["bindings"][0]["key"] = "Joystick";

    EXPECT_THROW((void)optionsFromJson(document), OptionsFormatError);
}

// Refused rather than repaired, exactly as a save's links are.
// A layout nobody could have chosen is not one to guess at.
TEST_F(OptionsFileTest, ADocumentBindingTwoActionsToOneKeyIsRefused)
{
    auto document = optionsToJson(PlayerOptions{});
    document["bindings"][1]["key"] =
        document["bindings"][0]["key"];

    EXPECT_THROW((void)optionsFromJson(document), OptionsFormatError);
}

TEST_F(OptionsFileTest, ADocumentBindingAReservedKeyIsRefused)
{
    auto document = optionsToJson(PlayerOptions{});
    document["bindings"][0]["key"] = "Escape";

    EXPECT_THROW((void)optionsFromJson(document), OptionsFormatError);
}

TEST_F(OptionsFileTest, AnUnwritablePathIsRefused)
{
    EXPECT_THROW(
        saveOptionsFile(PlayerOptions{}, pathIn("no/such/dir/o.json")),
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

    EXPECT_EQ(machine.bindings, rebound().bindings);
    EXPECT_EQ(machine.locale, rebound().locale);
    EXPECT_EQ(machine.path, pathIn("options.json"));
}

TEST_F(OptionsFileTest, ALiveRunWithNoFileYetIsToldTheShippedLayout)
{
    const auto machine =
        machineOptionsFor(false, pathIn("nothing-here.json"));

    EXPECT_EQ(machine.bindings, kDefaultBindings);
    EXPECT_EQ(machine.path, pathIn("nothing-here.json"));
}

// Version 2 added the language.
// A file written before there was a choice played the shipped one.
// And now says so, which is the whole of what the step does.
// That is what lets the schema require the member.
TEST_F(OptionsFileTest, AVersionOneDocumentIsReadAsTheShippedLanguage)
{
    auto document = optionsToJson(rebound());

    // Back to what a version 1 writer would have left behind.
    document.erase(std::string(antwika::game::kLocaleKey));
    document[versionKey()] = 1U;

    const auto loaded = optionsFromJson(document);

    EXPECT_EQ(loaded.locale, antwika::i18n::kDefaultLocale);

    // And the half that was already there is untouched by the step.
    EXPECT_EQ(loaded.bindings, rebound().bindings);
}

TEST_F(OptionsFileTest, APickedLanguageSurvivesTheFile)
{
    saveOptionsFile(
        PlayerOptions{.locale = antwika::i18n::Locale::Swedish},
        pathIn("options.json"));

    EXPECT_EQ(
        loadOptionsFileOrDefaults(pathIn("options.json")).locale,
        antwika::i18n::Locale::Swedish);
}

TEST_F(OptionsFileTest, APickedKeyboardSurvivesTheFile)
{
    saveOptionsFile(
        PlayerOptions{
            .keyboard = antwika::game::KeyboardLayout::English},
        pathIn("options.json"));

    EXPECT_EQ(
        loadOptionsFileOrDefaults(pathIn("options.json")).keyboard,
        antwika::game::KeyboardLayout::English);
}

// Version 3 added the board the typing is read off.
// A file written before there was a choice typed by the shipped one.
// And now says so, which is the whole of what the step does.
TEST_F(OptionsFileTest, AVersionTwoDocumentIsReadAsTheShippedBoard)
{
    auto document = optionsToJson(rebound());

    // Back to what a version 2 writer would have left behind.
    document.erase(std::string(antwika::game::kKeyboardKey));
    document[versionKey()] = 2U;

    const auto loaded = optionsFromJson(document);

    EXPECT_EQ(
        loaded.keyboard, antwika::game::kDefaultKeyboardLayout);

    // And the halves that were already there are untouched.
    EXPECT_EQ(loaded.bindings, rebound().bindings);
    EXPECT_EQ(loaded.locale, antwika::i18n::Locale::Swedish);
}

TEST_F(OptionsFileTest, ADocumentNamingNoBoardIsRefused)
{
    auto document = optionsToJson(PlayerOptions{});
    document[std::string(antwika::game::kKeyboardKey)] = "dvorak";

    EXPECT_THROW((void)optionsFromJson(document), OptionsFormatError);
}

TEST_F(OptionsFileTest, ADocumentNamingNoCatalogueIsRefused)
{
    auto document = optionsToJson(PlayerOptions{});
    document[std::string(antwika::game::kLocaleKey)] = "de";

    EXPECT_THROW((void)optionsFromJson(document), OptionsFormatError);
}

TEST_F(OptionsFileTest, ADocumentWithNoLanguageAtAllIsRefused)
{
    auto document = optionsToJson(PlayerOptions{});
    document.erase(std::string(antwika::game::kLocaleKey));

    EXPECT_THROW((void)optionsFromJson(document), OptionsFormatError);
}

// A replay reads neither half, for one reason.
TEST_F(OptionsFileTest, AReplayIsToldNothingAboutTheMachinesLanguage)
{
    saveOptionsFile(
        PlayerOptions{.locale = antwika::i18n::Locale::Swedish},
        pathIn("options.json"));

    const auto machine = machineOptionsFor(true, pathIn("options.json"));

    EXPECT_FALSE(machine.locale.has_value());
    EXPECT_FALSE(machine.bindings.has_value());
}
