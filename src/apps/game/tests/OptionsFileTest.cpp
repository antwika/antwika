#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <antwika/input/Key.hpp>
#include <antwika/replay/SchemaVersion.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/i18n/Locale.hpp>

#include "antwika/game/Action.hpp"
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
}

TEST_F(OptionsFileTest, OptionsToJson_ADocumentStatesItsFormatAndItsVersion)
{
    const auto encoded = optionsToJson(PlayerOptions{});

    EXPECT_EQ(encoded.at("magic").get<std::string>(), kOptionsMagic);
    EXPECT_EQ(
        encoded.at(versionKey()).get<std::uint32_t>(),
        kOptionsFormatVersion);
}

TEST_F(OptionsFileTest, OptionsFromJson_ALayoutRoundTripsThroughTheDocument)
{
    EXPECT_EQ(optionsFromJson(optionsToJson(rebound())), rebound());
    EXPECT_EQ(
        optionsFromJson(optionsToJson(PlayerOptions{})),
        PlayerOptions{});
}

TEST_F(OptionsFileTest, ReadOptions_ALayoutRoundTripsThroughAStream)
{
    std::stringstream stream;
    writeOptions(rebound(), stream);

    EXPECT_EQ(readOptions(stream), rebound());
}

TEST_F(OptionsFileTest, LoadOptionsFileOrDefaults_ALayoutRoundTripsThroughAFile)
{
    saveOptionsFile(rebound(), pathIn("options.json"));

    EXPECT_EQ(
        loadOptionsFileOrDefaults(pathIn("options.json")), rebound());
}

TEST_F(OptionsFileTest,
     LoadOptionsFileOrDefaults_AMissingFileIsAnOrdinaryFirstRun)
{
    EXPECT_EQ(
        loadOptionsFileOrDefaults(pathIn("nothing-here.json")),
        PlayerOptions{});
}

TEST_F(OptionsFileTest, LoadOptionsFileOrDefaults_TextThatIsNotJsonIsRefused)
{
    writeText("options.json", "not json at all");

    EXPECT_THROW(
        (void)loadOptionsFileOrDefaults(pathIn("options.json")),
        OptionsFormatError);
}

TEST_F(OptionsFileTest, OptionsFromJson_ADocumentOfAnotherFormatIsRefused)
{
    auto document = optionsToJson(PlayerOptions{});
    document["magic"] = "antwika-game-save";

    EXPECT_THROW((void)optionsFromJson(document), OptionsFormatError);
}

TEST_F(OptionsFileTest, OptionsFromJson_ADocumentFromANewerBuildIsRefused)
{
    auto document = optionsToJson(PlayerOptions{});
    document[versionKey()] = kOptionsFormatVersion + 1;

    EXPECT_THROW((void)optionsFromJson(document), OptionsFormatError);
}

TEST_F(OptionsFileTest, OptionsFromJson_ADocumentOfTheWrongShapeIsRefused)
{
    auto document = optionsToJson(PlayerOptions{});
    document["bindings"][0].erase("key");

    EXPECT_THROW((void)optionsFromJson(document), OptionsFormatError);
}

TEST_F(OptionsFileTest, OptionsFromJson_RefusesAnUnknownAction)
{
    auto document = optionsToJson(PlayerOptions{});
    document["bindings"][0]["action"] = "fly";

    EXPECT_THROW((void)optionsFromJson(document), OptionsFormatError);
}

TEST_F(OptionsFileTest, OptionsFromJson_RefusesAnUnknownKey)
{
    auto document = optionsToJson(PlayerOptions{});
    document["bindings"][0]["key"] = "Joystick";

    EXPECT_THROW((void)optionsFromJson(document), OptionsFormatError);
}

TEST_F(OptionsFileTest, OptionsFromJson_RefusesADoubleBoundKey)
{
    auto document = optionsToJson(PlayerOptions{});
    document["bindings"][1]["key"] =
        document["bindings"][0]["key"];

    EXPECT_THROW((void)optionsFromJson(document), OptionsFormatError);
}

TEST_F(OptionsFileTest, OptionsFromJson_ADocumentBindingAReservedKeyIsRefused)
{
    auto document = optionsToJson(PlayerOptions{});
    document["bindings"][0]["key"] = "Escape";

    EXPECT_THROW((void)optionsFromJson(document), OptionsFormatError);
}

TEST_F(OptionsFileTest, SaveOptionsFile_AnUnwritablePathIsRefused)
{
    EXPECT_THROW(
        saveOptionsFile(PlayerOptions{}, pathIn("no/such/dir/o.json")),
        OptionsFormatError);
}

TEST_F(OptionsFileTest, SaveOptionsFileIfNamed_NamingNowhereWritesNothing)
{
    saveOptionsFileIfNamed(rebound(), std::nullopt);

    saveOptionsFileIfNamed(rebound(), pathIn("options.json"));
    EXPECT_TRUE(std::filesystem::exists(pathIn("options.json")));
}

TEST_F(OptionsFileTest, StandardOptionsMigrations_ReachCurrent)
{
    EXPECT_EQ(
        standardOptionsMigrations().currentVersion(),
        kOptionsFormatVersion);
}

TEST_F(OptionsFileTest, MachineOptionsFor_HidesTheMachineOnReplay)
{
    saveOptionsFile(rebound(), pathIn("options.json"));

    const auto machine = machineOptionsFor(true, pathIn("options.json"));

    EXPECT_FALSE(machine.bindings.has_value());
    EXPECT_FALSE(machine.path.has_value());
}

TEST_F(OptionsFileTest, MachineOptionsFor_TellsALiveRunTheFile)
{
    saveOptionsFile(rebound(), pathIn("options.json"));

    const auto machine = machineOptionsFor(false, pathIn("options.json"));

    EXPECT_EQ(machine.bindings, rebound().bindings);
    EXPECT_EQ(machine.locale, rebound().locale);
    EXPECT_EQ(machine.path, pathIn("options.json"));
}

TEST_F(OptionsFileTest, MachineOptionsFor_FallsBackToShipped)
{
    const auto machine =
        machineOptionsFor(false, pathIn("nothing-here.json"));

    EXPECT_EQ(machine.bindings, kDefaultBindings);
    EXPECT_EQ(machine.path, pathIn("nothing-here.json"));
}

TEST_F(OptionsFileTest, OptionsFromJson_ReadsVersionOneAsShipped)
{
    auto document = optionsToJson(rebound());

    document.erase(std::string(antwika::game::kLocaleKey));
    document[versionKey()] = 1U;

    const auto loaded = optionsFromJson(document);

    EXPECT_EQ(loaded.locale, antwika::i18n::kDefaultLocale);

    EXPECT_EQ(loaded.bindings, rebound().bindings);
}

TEST_F(OptionsFileTest,
     LoadOptionsFileOrDefaults_APickedLanguageSurvivesTheFile)
{
    saveOptionsFile(
        PlayerOptions{.locale = antwika::i18n::Locale::Swedish},
        pathIn("options.json"));

    EXPECT_EQ(
        loadOptionsFileOrDefaults(pathIn("options.json")).locale,
        antwika::i18n::Locale::Swedish);
}

TEST_F(OptionsFileTest,
     LoadOptionsFileOrDefaults_APickedKeyboardSurvivesTheFile)
{
    saveOptionsFile(
        PlayerOptions{
            .keyboard = antwika::game::KeyboardLayout::English},
        pathIn("options.json"));

    EXPECT_EQ(
        loadOptionsFileOrDefaults(pathIn("options.json")).keyboard,
        antwika::game::KeyboardLayout::English);
}

TEST_F(OptionsFileTest, OptionsFromJson_ReadsVersionTwoAsShipped)
{
    auto document = optionsToJson(rebound());

    document.erase(std::string(antwika::game::kKeyboardKey));
    document[versionKey()] = 2U;

    const auto loaded = optionsFromJson(document);

    EXPECT_EQ(
        loaded.keyboard, antwika::game::kDefaultKeyboardLayout);

    EXPECT_EQ(loaded.bindings, rebound().bindings);
    EXPECT_EQ(loaded.locale, antwika::i18n::Locale::Swedish);
}

TEST_F(OptionsFileTest, OperatorEquals_SeparatesADifferentBoard)
{
    PlayerOptions base;
    PlayerOptions other;
    other.keyboard = antwika::game::KeyboardLayout::English;

    EXPECT_NE(base, other);
}

TEST_F(OptionsFileTest, OptionsFromJson_ADocumentNamingNoBoardIsRefused)
{
    auto document = optionsToJson(PlayerOptions{});
    document[std::string(antwika::game::kKeyboardKey)] = "dvorak";

    EXPECT_THROW((void)optionsFromJson(document), OptionsFormatError);
}

TEST_F(OptionsFileTest, OptionsFromJson_ADocumentNamingNoCatalogueIsRefused)
{
    auto document = optionsToJson(PlayerOptions{});
    document[std::string(antwika::game::kLocaleKey)] = "de";

    EXPECT_THROW((void)optionsFromJson(document), OptionsFormatError);
}

TEST_F(OptionsFileTest, OptionsFromJson_RefusesNoLanguage)
{
    auto document = optionsToJson(PlayerOptions{});
    document.erase(std::string(antwika::game::kLocaleKey));

    EXPECT_THROW((void)optionsFromJson(document), OptionsFormatError);
}

TEST_F(OptionsFileTest, MachineOptionsFor_HidesTheLanguageOnReplay)
{
    saveOptionsFile(
        PlayerOptions{.locale = antwika::i18n::Locale::Swedish},
        pathIn("options.json"));

    const auto machine = machineOptionsFor(true, pathIn("options.json"));

    EXPECT_FALSE(machine.locale.has_value());
    EXPECT_FALSE(machine.bindings.has_value());
}
