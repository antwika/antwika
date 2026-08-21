#include "antwika/editor/plan/PlanFile.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <string>

#include <antwika/schema/JsonSchemas.hpp>
#include <antwika/schema/MigrationChain.hpp>
#include <antwika/schema/SchemaVersion.hpp>
#include <antwika/schema/VersionedDocument.hpp>
#include <antwika/enums/Enumeration.hpp>
#include <antwika/io/File.hpp>
#include <antwika/io/SafeWrite.hpp>

#include "antwika/editor/plan/PlanFileError.hpp"

namespace antwika::editor
{

    namespace
    {
        constexpr int kIndent = 2;

        constexpr std::string_view kMagicKey = "magic";

        constexpr std::string_view kColumnsKey = "columns";

        constexpr std::string_view kTitleKey = "title";

        constexpr std::string_view kBodyKey = "body";

        constexpr std::string_view kFailed =
            "antwika: could not read the plan: ";

        constexpr std::array<std::string_view, 3> kColumnKeys{
            "todo", "doing", "done"};

        [[nodiscard]] nlohmann::json cardSchema()
        {
            auto shape = schema::objectSchema({kTitleKey, kBodyKey});
            shape["properties"][std::string(kTitleKey)] =
                schema::wordSchema();
            shape["properties"][std::string(kBodyKey)] =
                schema::wordSchema();

            return shape;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] nlohmann::json columnsSchema()
        {
            auto shape = schema::objectSchema(
                {kColumnKeys[0], kColumnKeys[1], kColumnKeys[2]});

            for (const auto key : kColumnKeys)
            {
                auto list = nlohmann::json{};
                list["type"] = "array";
                list["maxItems"] = kMaxCardsPerColumn;
                list["items"] = cardSchema();
                shape["properties"][std::string(key)] = list;
            }

            return shape;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] nlohmann::json planSchema()
        {
            auto shape = schema::documentSchema(
                "antwika plan",
                {kMagicKey, schema::kSchemaVersionKey, kColumnsKey});
            shape["properties"][std::string(kMagicKey)]["const"] =
                std::string(kPlanMagic);
            shape["properties"]
                 [std::string(schema::kSchemaVersionKey)] =
                     schema::boundedCountSchema(kPlanVersion);
            shape["properties"][std::string(kColumnsKey)] =
                columnsSchema();

            return shape;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] const schema::MigrationChain &planMigrations()
        {
            static const schema::MigrationChain chain(
                schema::MigrationList{}, kPlanVersion);

            return chain;
        }
    }

    void writeBoard(std::ostream &outputStream, const Board &board)
    {
        nlohmann::json document;

        document[std::string(kMagicKey)] = std::string(kPlanMagic);
        document[std::string(schema::kSchemaVersionKey)] = kPlanVersion;

        for (const auto which : kEveryColumn)
        {
            auto arrayJson = nlohmann::json::array();

            for (const auto &card : cardsOf(board, which))
            {
                nlohmann::json one;
                one[std::string(kTitleKey)] = card.title;
                one[std::string(kBodyKey)] = card.body;
                arrayJson.push_back(std::move(one));
            }

            document[std::string(kColumnsKey)]
                    [std::string(kColumnKeys.at(enums::index(which)))] =
                        std::move(arrayJson);
        }

        outputStream << document.dump(kIndent) << '\n';
    }

    Board readBoard(std::istream &inputStream)
    {
        nlohmann::json document;

        try
        {
            inputStream >> document;
        }
        catch (const nlohmann::json::exception &error)
        {
            throw PlanFileError(
                std::string(kFailed) + error.what());
        }

        const auto wholeDocument = schema::readVersionedDocument<PlanFileError>(
            document, planMigrations(), schema::validatorFor<planSchema>(),
            kFailed);

        Board board;

        for (const auto which : kEveryColumn)
        {
            const auto &columnsJson =
                wholeDocument[std::string(kColumnsKey)][std::string(
                    kColumnKeys.at(enums::index(which)))];

            for (const auto &one : columnsJson)
            {
                board.columnCards.at(enums::index(which))
                    .push_back(Card{
                        .title = one[std::string(kTitleKey)]
                                     .get<std::string>(),
                        .body = one[std::string(kBodyKey)]
                                    .get<std::string>()});
            }
        }

        return board;
    }

    void saveBoard(const std::string &path, const Board &board)
    {
        const auto writingPath = io::writingPathFor(path);

        {
            auto outputStream = io::openToWriteAs<PlanFileError>(
                writingPath, "the plan");

            writeBoard(outputStream, board);
            io::requireStreamOkAs<PlanFileError>(
                outputStream, "the plan", writingPath);
        }

        io::putInPlaceKeepingBackup<PlanFileError>(
            writingPath, path, "the plan");
    }

    std::optional<Board> loadBoard(const std::string &path)
    {
        auto inputStream = io::openToReadIfPresent(path);

        if (!inputStream.has_value())
        {
            return std::nullopt;
        }

        return readBoard(*inputStream);
    }

}
