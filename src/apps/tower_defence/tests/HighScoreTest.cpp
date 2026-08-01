#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <antwika/replay/SchemaVersion.hpp>

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
    // Removes its backing file on scope exit.
    // That way a failing assertion leaves no stray temp files behind.
    class ScratchFile
    {
    public:
        explicit ScratchFile(std::string_view name)
            : path(std::filesystem::temp_directory_path() / name)
        {
        }

        ~ScratchFile()
        {
            // The error_code overload, not the throwing one.
            // A destructor is implicitly noexcept.
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
        }

        ScratchFile(const ScratchFile &) = delete;
        ScratchFile(ScratchFile &&) = delete;
        ScratchFile &operator=(const ScratchFile &) = delete;
        ScratchFile &operator=(ScratchFile &&) = delete;

        [[nodiscard]] std::string string() const
        {
            return path.string();
        }

    private:
        std::filesystem::path path;
    };

    HighScore parsed(const std::string &text)
    {
        std::istringstream in(text);
        return readHighScore(in);
    }

    TEST(HighScoreTest, ARecordSurvivesARoundTripThroughItsFormat)
    {
        const HighScore kept{.bestScore = 1234, .bestLevel = 3};

        std::ostringstream out;
        writeHighScore(kept, out);

        std::istringstream in(out.str());
        EXPECT_EQ(readHighScore(in), kept);
    }

    TEST(HighScoreTest, TheDocumentStatesItsFormatAndItsVersion)
    {
        const auto document =
            highScoreToJson(HighScore{.bestScore = 7, .bestLevel = 1});

        EXPECT_EQ(document.at("magic"), std::string(kScoreMagic));
        EXPECT_EQ(
            document.at(
                std::string(antwika::replay::kSchemaVersionKey)),
            kScoreFormatVersion);
    }

    // The chain has no steps yet, since this format has one revision.
    // What it still does is refuse a file from a newer build.
    TEST(HighScoreTest, TheChainIsAtTheCurrentVersionWithNoStepsInIt)
    {
        EXPECT_EQ(
            standardScoreMigrations().currentVersion(),
            kScoreFormatVersion);
    }

    TEST(HighScoreTest, ADocumentWithNoVersionIsReadAsTheFirstOne)
    {
        nlohmann::json document;
        document["magic"] = std::string(kScoreMagic);
        document["bestScore"] = 90;
        document["bestLevel"] = 2;

        const HighScore decoded = highScoreFromJson(document);
        EXPECT_EQ(decoded.bestScore, 90U);
        EXPECT_EQ(decoded.bestLevel, 2U);
    }

    TEST(HighScoreTest, AVersionThisBuildCannotReachIsRefused)
    {
        nlohmann::json document =
            highScoreToJson(HighScore{.bestScore = 1, .bestLevel = 1});
        document[std::string(antwika::replay::kSchemaVersionKey)] =
            kScoreFormatVersion + 1;

        EXPECT_THROW(
            static_cast<void>(highScoreFromJson(document)),
            ScoreFormatError);
    }

    TEST(HighScoreTest, TextThatIsNotJsonIsRefused)
    {
        EXPECT_THROW(
            static_cast<void>(parsed("not a document at all")),
            ScoreFormatError);
    }

    TEST(HighScoreTest, AnotherFormatsDocumentIsRefused)
    {
        EXPECT_THROW(
            static_cast<void>(parsed(
                R"({"magic":"antwika-companion","version":1,)"
                R"("bestScore":1,"bestLevel":1})")),
            ScoreFormatError);
    }

    TEST(HighScoreTest, AMissingMemberIsRefused)
    {
        EXPECT_THROW(
            static_cast<void>(parsed(
                R"({"magic":"antwika-tower-defence-score",)"
                R"("version":1,"bestScore":1})")),
            ScoreFormatError);
    }

    // Two records are the same one only if both halves agree.
    TEST(HighScoreTest, ARecordDiffersOnEitherOfItsTwoNumbers)
    {
        const HighScore kept{.bestScore = 100, .bestLevel = 2};

        EXPECT_EQ(kept, (HighScore{.bestScore = 100, .bestLevel = 2}));
        EXPECT_NE(kept, (HighScore{.bestScore = 101, .bestLevel = 2}));
        EXPECT_NE(kept, (HighScore{.bestScore = 100, .bestLevel = 3}));
    }

    TEST(HighScoreTest, TheHigherScoreIsTheOneKept)
    {
        const HighScore old{.bestScore = 100, .bestLevel = 2};

        EXPECT_EQ(
            bestOf(old, HighScore{.bestScore = 150, .bestLevel = 1}),
            (HighScore{.bestScore = 150, .bestLevel = 1}));
        EXPECT_EQ(
            bestOf(old, HighScore{.bestScore = 60, .bestLevel = 3}), old);

        // A tie keeps what was already there rather than replacing it.
        EXPECT_EQ(
            bestOf(old, HighScore{.bestScore = 100, .bestLevel = 3}),
            old);
    }

    TEST(FileScoreStoreTest, AFileThatIsNotThereIsAFirstRun)
    {
        const ScratchFile file("antwika-td-absent.json");
        FileScoreStore store(file.string());
        EXPECT_FALSE(store.load().has_value());
    }

    TEST(FileScoreStoreTest, WhatIsWrittenIsWhatIsReadBack)
    {
        const ScratchFile file("antwika-td-record.json");
        const HighScore kept{.bestScore = 512, .bestLevel = 2};

        FileScoreStore store(file.string());
        store.save(kept);

        const auto loaded = store.load();
        ASSERT_TRUE(loaded.has_value());
        EXPECT_EQ(*loaded, kept);
    }

    TEST(FileScoreStoreTest, AFileThatWillNotReadIsReported)
    {
        const ScratchFile file("antwika-td-broken.json");
        {
            std::ofstream out(file.string());
            out << "{ this is not a record";
        }

        FileScoreStore store(file.string());
        EXPECT_THROW(
            static_cast<void>(store.load()), ScoreFormatError);
    }

    TEST(FileScoreStoreTest, APathThatCannotBeOpenedIsReported)
    {
        FileScoreStore store("/antwika-no-such-directory/record.json");
        EXPECT_THROW(store.save(HighScore{}), ScoreFormatError);
    }

    // A full disk fails only once the bytes are flushed.
    // /dev/full is the portable-enough way to make that happen.
    TEST(FileScoreStoreTest, BytesThatCannotBeWrittenAreReported)
    {
        if (!std::filesystem::exists("/dev/full"))
        {
            GTEST_SKIP() << "no /dev/full to fill";
        }

        FileScoreStore store("/dev/full");
        EXPECT_THROW(
            store.save(HighScore{.bestScore = 1, .bestLevel = 1}),
            ScoreFormatError);
    }
} // namespace
