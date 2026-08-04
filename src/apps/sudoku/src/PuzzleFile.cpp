#include "antwika/sudoku/PuzzleFile.hpp"

#include <cctype>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include <nlohmann/json-schema.hpp>

#include <antwika/config/ConfigDocument.hpp>
#include <antwika/replay/IMigration.hpp>
#include <antwika/replay/SchemaVersion.hpp>
#include <antwika/replay/VersionedDocument.hpp>

#include "antwika/sudoku/BoardFormatError.hpp"

namespace antwika::sudoku
{

    namespace
    {
        // What a version 1 puzzle becomes.
        //
        // Version 1 is 81 characters, with no room to say what it is.
        // Every other document here states its version in one member.
        // Two of them would satisfy a schema asking only for a grid.
        // So version 2 says which format it is.
        // And this is where a grid written before that gains it.
        class PuzzleV1ToV2 final : public antwika::replay::IMigration
        {
        public:
            [[nodiscard]] std::uint32_t fromVersion() const noexcept
                override
            {
                return 1;
            }

            [[nodiscard]] std::uint32_t toVersion() const noexcept
                override
            {
                return 2;
            }

            // MigrationChain asks for this in one place only.
            // It is the message thrown when a step is not one step.
            // This one reads 1 and produces 2.
            // So reaching it means editing the two functions above.
            // Which breaks the migration rather than feeding it input.
            // See docs/confirming-unreachable-branches.md.
            // GCOVR_EXCL_START
            [[nodiscard]] std::string_view name() const noexcept override
            {
                return "sudoku: a grid starts saying what it is";
            }
            // GCOVR_EXCL_STOP

            void apply(nlohmann::json &document) const override
            {
                document["magic"] = std::string(kPuzzleMagic);
            }
        };

        // "cells" is a plain string here rather than a pattern.
        // Board::parse() is what judges the 81 characters.
        // It says which character it did not like, and where.
        // companion::PetSave draws that line around its own members.
        nlohmann::json puzzleSchema()
        {
            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "antwika sudoku puzzle document";
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            schema["required"] = {"magic", "cells"}; // GCOVR_EXCL_LINE
            schema["properties"]["magic"]["const"] =
                std::string(kPuzzleMagic);
            schema["properties"]
                  [std::string(antwika::replay::kSchemaVersionKey)]
                  ["const"] = kPuzzleDocumentVersion;
            schema["properties"]["cells"]["type"] = "string";
            return schema;
        }

        const nlohmann::json_schema::json_validator &puzzleValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                puzzleSchema()); // GCOVR_EXCL_LINE
            return validator;
        }

        [[nodiscard]] bool opensLikeJson(const std::string_view text)
        {
            for (const char c : text)
            {
                if (std::isspace(static_cast<unsigned char>(c)) != 0)
                {
                    continue;
                }

                return c == '{';
            }

            return false;
        }
    } // namespace

    MigrationChain standardPuzzleMigrations()
    {
        // The version key is the shared one, so none is passed.
        antwika::replay::MigrationList migrations;
        migrations.push_back(std::make_shared<const PuzzleV1ToV2>());

        return MigrationChain(
            std::move(migrations), kPuzzleDocumentVersion);
    }

    nlohmann::json parsePuzzleDocument(const std::string_view text)
    {
        if (!opensLikeJson(text))
        {
            // gcov puts this block's cleanup on these lines.
            // Building a document that owns storage is what makes one.
            // No input reaches it, exactly as in ReplayJson's encoder.
            // See docs/confirming-unreachable-branches.md.
            // GCOVR_EXCL_START
            nlohmann::json document;
            document["cells"] = std::string(text);
            return document;
        }
        // GCOVR_EXCL_STOP

        try
        {
            return nlohmann::json::parse(text);
        }
        catch (const nlohmann::json::exception &error) // GCOVR_EXCL_LINE
        {
            throw BoardFormatError(
                std::string("antwika::sudoku: a puzzle document opens "
                            "like JSON and is not: ")
                + error.what());
        }
    } // GCOVR_EXCL_LINE

    Board puzzleFromJson(const nlohmann::json &document)
    {
        const auto migrated =
            antwika::config::migratedAs<BoardFormatError>(
                document,
                standardPuzzleMigrations(),
                puzzleValidator(),
                "antwika::sudoku: a puzzle failed schema validation: ");

        return Board::parse(migrated.at("cells").get<std::string>());
    }

    Board readPuzzle(std::istream &in)
    {
        std::ostringstream contents;
        contents << in.rdbuf();

        return puzzleFromJson(parsePuzzleDocument(contents.str()));
    }

    std::optional<Board> startingPuzzle(
        const std::optional<std::string> &puzzlePath,
        const bool replaying)
    {
        if (replaying)
        {
            return std::nullopt;
        }

        if (!puzzlePath.has_value())
        {
            return Board::parse(kDemoPuzzle);
        }

        std::ifstream file{*puzzlePath};

        // Unchecked, a missing file read as an empty puzzle.
        // Which Board::parse then reported as the wrong length.
        if (!file.is_open())
        {
            throw BoardFormatError(
                "antwika::sudoku: could not open a puzzle: "
                + *puzzlePath);
        }

        return readPuzzle(file);
    }

} // namespace antwika::sudoku
