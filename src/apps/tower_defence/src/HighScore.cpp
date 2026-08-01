#include "antwika/tower_defence/HighScore.hpp"

#include <string>

#include <nlohmann/json-schema.hpp>

#include <antwika/replay/JsonShapes.hpp>
#include <antwika/replay/SchemaVersion.hpp>
#include <antwika/replay/VersionedDocument.hpp>

#include "antwika/tower_defence/ScoreFormatError.hpp"

namespace antwika::tower_defence
{

    namespace
    {
        // Two spaces, one member a line.
        // Enough to diff a record against the next one.
        // That is the whole reason this format is not compact.
        constexpr int kIndent = 2;

        using antwika::replay::countShape;

        nlohmann::json scoreSchema()
        {
            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "antwika tower defence score document";
            schema["type"] = "object";
            schema["additionalProperties"] = false;

            // The version member is described but not required.
            // A document without one is read as version 1 instead.
            // By the time this runs the document has been migrated.
            // So the only version it may carry is the current one.
            // GCOVR_EXCL_START
            schema["required"] = {"magic", "bestScore", "bestLevel"};
            // GCOVR_EXCL_STOP
            schema["properties"]["magic"]["const"] =
                std::string(kScoreMagic);
            schema["properties"][std::string(replay::kSchemaVersionKey)]
                  ["const"] = kScoreFormatVersion;
            schema["properties"]["bestScore"] = countShape();
            schema["properties"]["bestLevel"] = countShape();
            return schema;
        }

        const nlohmann::json_schema::json_validator &scoreValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                scoreSchema()); // GCOVR_EXCL_LINE
            return validator;
        }
    } // namespace

    MigrationChain standardScoreMigrations()
    {
        // The version key is the shared one, so none is passed.
        // The list is empty because this format has one revision.
        return MigrationChain({}, kScoreFormatVersion);
    }

    nlohmann::json highScoreToJson(const HighScore &score)
    {
        nlohmann::json encoded;
        encoded["magic"] = std::string(kScoreMagic);
        encoded[std::string(replay::kSchemaVersionKey)] =
            kScoreFormatVersion;
        encoded["bestScore"] = score.bestScore;
        encoded["bestLevel"] = score.bestLevel;
        return encoded;

        // gcov puts the cleanup block on this closing brace.
        // ReplayJson.cpp's own encoder explains it at length.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    HighScore highScoreFromJson(const nlohmann::json &document)
    {
        const auto migrated =
            replay::readVersionedDocument<ScoreFormatError>(
                document,
                standardScoreMigrations(),
                scoreValidator(),
                "antwika::tower_defence: a saved high score failed "
                "schema validation: ");

        return HighScore{
            .bestScore = migrated.at("bestScore").get<std::uint64_t>(),
            .bestLevel = migrated.at("bestLevel").get<std::size_t>()};
    }

    void writeHighScore(const HighScore &score, std::ostream &out)
    {
        out << highScoreToJson(score).dump(kIndent) << '\n';
    }

    HighScore readHighScore(std::istream &in)
    {
        nlohmann::json document;
        try
        {
            in >> document;
        }
        catch (const nlohmann::json::exception &error) // GCOVR_EXCL_LINE
        {
            throw ScoreFormatError(
                std::string("antwika::tower_defence: a saved high score "
                            "is not valid JSON: ")
                + error.what());
        }

        return highScoreFromJson(document);
    }

    HighScore bestOf(const HighScore &best, const HighScore &run)
    {
        if (run.bestScore <= best.bestScore)
        {
            return best;
        }

        return run;
    }

} // namespace antwika::tower_defence
