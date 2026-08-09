#include "antwika/sudoku/PuzzleFile.hpp"

#include <cctype>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include <antwika/config/ConfigDocument.hpp>
#include <antwika/replay/IMigration.hpp>
#include <antwika/replay/JsonShapes.hpp>
#include <antwika/replay/SchemaVersion.hpp>
#include <antwika/replay/VersionedDocument.hpp>

#include "antwika/sudoku/BoardFormatError.hpp"

namespace antwika::sudoku
{

    namespace
    {
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

        nlohmann::json puzzleSchema()
        {
            nlohmann::json schema = antwika::replay::documentShape(
                "antwika sudoku puzzle document", {"magic", "cells"});
            schema["properties"]["magic"]["const"] =
                std::string(kPuzzleMagic);
            schema["properties"]
                  [std::string(antwika::replay::kSchemaVersionKey)]
                  ["const"] = kPuzzleDocumentVersion;
            schema["properties"]["cells"] = antwika::replay::wordShape();
            return schema;
        } // GCOVR_EXCL_LINE

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
    }

    MigrationChain standardPuzzleMigrations()
    {
        antwika::replay::MigrationList migrations;
        migrations.push_back(std::make_shared<const PuzzleV1ToV2>());

        return MigrationChain(
            std::move(migrations), kPuzzleDocumentVersion);
    }

    nlohmann::json parsePuzzleDocument(const std::string_view text)
    {
        if (!opensLikeJson(text))
        {
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
                antwika::replay::validatorFor<puzzleSchema>(),
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

        if (!file.is_open())
        {
            throw BoardFormatError(
                "antwika::sudoku: could not open a puzzle: "
                + *puzzlePath);
        }

        return readPuzzle(file);
    }

}
