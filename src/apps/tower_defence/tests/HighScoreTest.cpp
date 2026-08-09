#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unistd.h>

#include <antwika/replay/SchemaVersion.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include "antwika/tower_defence/FileScoreStore.hpp"
#include "antwika/tower_defence/HighScore.hpp"
#include "antwika/tower_defence/ScoreFormatError.hpp"

using antwika::tower_defence::bestOf;
using antwika::tower_defence::FileScoreStore;
using antwika::tower_defence::HighScore;
using antwika::tower_defence::highScoreFromJson;
using antwika::tower_defence::highScoreToJson;
using antwika::tower_defence::kScoreFormatVersion;
using antwika::tower_defence::kScoreMagic;
using antwika::tower_defence::readHighScore;
using antwika::tower_defence::ScoreFormatError;
using antwika::tower_defence::standardScoreMigrations;
using antwika::tower_defence::writeHighScore;

namespace
{
    HighScore parsed(const std::string &text)
    {
        std::istringstream in(text);
        return readHighScore(in);
    }

    TEST(HighScoreTest, WriteHighScore_RoundTripsThroughRead)
    {
        const HighScore kept{.bestScore = 1234, .bestLevel = 3};

        std::ostringstream out;
        writeHighScore(kept, out);

        std::istringstream in(out.str());
        EXPECT_EQ(readHighScore(in), kept);
    }

    TEST(HighScoreTest, HighScoreToJson_StatesFormatAndVersion)
    {
        const auto document =
            highScoreToJson(HighScore{.bestScore = 7, .bestLevel = 1});

        EXPECT_EQ(document.at("magic"), std::string(kScoreMagic));
        EXPECT_EQ(
            document.at(
                std::string(antwika::replay::kSchemaVersionKey)),
            kScoreFormatVersion);
    }

    TEST(HighScoreTest, StandardScoreMigrations_AreAtCurrent)
    {
        EXPECT_EQ(
            standardScoreMigrations().currentVersion(),
            kScoreFormatVersion);
    }

    TEST(HighScoreTest, HighScoreFromJson_ReadsNoVersionAsOne)
    {
        nlohmann::json document;
        document["magic"] = std::string(kScoreMagic);
        document["bestScore"] = 90;
        document["bestLevel"] = 2;

        const HighScore decoded = highScoreFromJson(document);
        EXPECT_EQ(decoded.bestScore, 90U);
        EXPECT_EQ(decoded.bestLevel, 2U);
    }

    TEST(HighScoreTest, HighScoreFromJson_RefusesAFutureVersion)
    {
        nlohmann::json document =
            highScoreToJson(HighScore{.bestScore = 1, .bestLevel = 1});
        document[std::string(antwika::replay::kSchemaVersionKey)] =
            kScoreFormatVersion + 1;

        EXPECT_THROW(
            static_cast<void>(highScoreFromJson(document)),
            ScoreFormatError);
    }

    TEST(HighScoreTest, HighScoreFromJson_RefusesNonJson)
    {
        EXPECT_THROW(
            static_cast<void>(parsed("not a document at all")),
            ScoreFormatError);
    }

    TEST(HighScoreTest, HighScoreFromJson_RefusesAnotherFormat)
    {
        EXPECT_THROW(
            static_cast<void>(parsed(
                R"({"magic":"antwika-companion","version":1,)"
                R"("bestScore":1,"bestLevel":1})")),
            ScoreFormatError);
    }

    TEST(HighScoreTest, HighScoreFromJson_RefusesAMissingMember)
    {
        EXPECT_THROW(
            static_cast<void>(parsed(
                R"({"magic":"antwika-tower-defence-score",)"
                R"("version":1,"bestScore":1})")),
            ScoreFormatError);
    }

    TEST(HighScoreTest, OperatorEquals_SeparatesEitherOfTheTwoNumbers)
    {
        const HighScore kept{.bestScore = 100, .bestLevel = 2};

        EXPECT_EQ(kept, (HighScore{.bestScore = 100, .bestLevel = 2}));
        EXPECT_NE(kept, (HighScore{.bestScore = 101, .bestLevel = 2}));
        EXPECT_NE(kept, (HighScore{.bestScore = 100, .bestLevel = 3}));
    }

    TEST(HighScoreTest, BestOf_KeepsTheHigherScore)
    {
        const HighScore old{.bestScore = 100, .bestLevel = 2};

        EXPECT_EQ(
            bestOf(old, HighScore{.bestScore = 150, .bestLevel = 1}),
            (HighScore{.bestScore = 150, .bestLevel = 1}));
        EXPECT_EQ(
            bestOf(old, HighScore{.bestScore = 60, .bestLevel = 3}), old);

        EXPECT_EQ(
            bestOf(old, HighScore{.bestScore = 100, .bestLevel = 3}),
            old);
    }

    TEST(FileScoreStoreTest, Load_ReadsAMissingFileAsAFirstRun)
    {
        const antwika::testing::ScratchFile file("antwika-td-absent.json");
        FileScoreStore store(file.string());
        EXPECT_FALSE(store.load().has_value());
    }

    TEST(FileScoreStoreTest, Save_WritesWhatLoadReadsBack)
    {
        const antwika::testing::ScratchFile file("antwika-td-record.json");
        const HighScore kept{.bestScore = 512, .bestLevel = 2};

        FileScoreStore store(file.string());
        store.save(kept);

        const auto loaded = store.load();
        ASSERT_TRUE(loaded.has_value());
        EXPECT_EQ(*loaded, kept);
    }

    TEST(FileScoreStoreTest, Load_ReportsAFileThatWillNotRead)
    {
        const antwika::testing::ScratchFile file("antwika-td-broken.json");
        {
            std::ofstream out(file.string());
            out << "{ this is not a record";
        }

        FileScoreStore store(file.string());
        EXPECT_THROW(
            static_cast<void>(store.load()), ScoreFormatError);
    }

    TEST(FileScoreStoreTest, Save_ReportsAPathItCannotOpen)
    {
        FileScoreStore store("/antwika-no-such-directory/record.json");
        EXPECT_THROW(store.save(HighScore{}), ScoreFormatError);
    }
}
