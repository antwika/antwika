#include "antwika/replay/ReplayReader.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <format>
#include <string>
#include <utility>

#include <antwika/log/Level.hpp>
#include <antwika/schema/DocumentDepth.hpp>
#include <antwika/replay/ReplayFormatError.hpp>
#include <antwika/replay/ReplayHeader.hpp>
#include <antwika/replay/ReplayJson.hpp>

#include "ReplayFormat.hpp"

namespace antwika::replay
{

    using schema::exceedsMaxDepth;

    namespace
    {
        void warnIfCanvasDiffers(
            const CanvasCheckOptions &check, const ReplayDocument &document)
        {
            if (!check.canvasSize.has_value()
                || !document.canvasSize.has_value())
            {
                return;
            }
            if (*check.canvasSize == *document.canvasSize)
            {
                return;
            }
            if (!check.logger.has_value())
            {
                return;
            }

            check.logger->get().log(
                log::Level::Warning,
                std::format(
                    "antwika::replay: this replay was recorded against a "
                    "{}x{} canvas and is being replayed against {}x{}; "
                    "recorded input may land somewhere else",
                    document.canvasSize->width,
                    document.canvasSize->height,
                    check.canvasSize->width,
                    check.canvasSize->height));
        }

        [[nodiscard]] bool isBlank(const std::string &line)
        {
            return line.find_first_not_of(" \t\r\n") == std::string::npos;
        }

        [[nodiscard]] nlohmann::json readFirstValue(std::istream &inputStream)
        {
            nlohmann::json first;
            try
            {
                inputStream >> first;
            }
            catch (const nlohmann::json::parse_error &) // GCOVR_EXCL_LINE
            {
                throw ReplayFormatError(
                    "antwika::replay: not a valid replay stream (it does "
                    "not open with a JSON value)");
            }
            return first;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] nlohmann::json readRecordLines(std::istream &inputStream)
        {
            auto records = nlohmann::json::array();

            std::size_t line = 1;
            std::string text;
            while (std::getline(inputStream, text))
            {
                const std::size_t lineNumber = line;
                ++line;

                if (isBlank(text))
                {
                    continue;
                }

                try
                {
                    records.push_back(nlohmann::json::parse(text));
                }
                catch (const nlohmann::json::parse_error &) // GCOVR_EXCL_LINE
                {
                    if (inputStream.eof())
                    {
                        break;
                    }

                    throw ReplayFormatError(std::format(
                        "antwika::replay: line {} of this replay is not "
                        "a JSON value",
                        lineNumber));
                }

                if (exceedsMaxDepth(records.back()))
                {
                    throw ReplayFormatError(std::format(
                        "antwika::replay: line {} of this replay nests "
                        "deeper than the format ever writes",
                        lineNumber));
                }
            }
            return records;
        } // GCOVR_EXCL_LINE

        void requireNothingAfter(std::istream &inputStream)
        {
            std::string text;

            while (std::getline(inputStream, text))
            {
                if (isBlank(text))
                {
                    continue;
                }

                if (text.find(std::string(detail::kMagicKey))
                    != std::string::npos)
                {
                    throw ReplayFormatError(
                        "antwika::replay: a second header follows this "
                        "whole-document replay; a file holds one run");
                }

                throw ReplayFormatError(
                    "antwika::replay: content follows this "
                    "whole-document replay, and a version 1 document "
                    "holds the run entire");
            }
        }

        [[nodiscard]] ReplayDocument readStream(
            std::istream &inputStream, const MigrationChain &migrations)
        {
            const nlohmann::json first = readFirstValue(inputStream);

            if (exceedsMaxDepth(first))
            {
                throw ReplayFormatError(
                    "antwika::replay: the opening JSON value of this "
                    "replay nests deeper than the format ever writes");
            }

            const std::string events(detail::kLegacyEventsKey);
            if (first.is_object() && first.contains(events))
            {
                requireNothingAfter(inputStream);

                return replayFromJson(first, migrations);
            }

            const ReplayHeader header =
                replayHeaderFromJson(first, migrations);

            ReplayDocument document;
            document.canvasSize = header.canvasSize;
            document.events = replayRecordsFromJson(
                readRecordLines(inputStream), header.version, migrations);
            return document;
        } // GCOVR_EXCL_LINE
    }

    ReplayReader::ReplayReader(
        CanvasCheckOptions check, MigrationChain migrations)
        : check(std::move(check)), migrations(std::move(migrations))
    {
    }

    std::vector<TickEvent> ReplayReader::read(std::istream &inputStream) const
    {
        ReplayDocument document = readStream(inputStream, migrations);
        warnIfCanvasDiffers(check, document);
        return std::move(document.events);
    }

}
