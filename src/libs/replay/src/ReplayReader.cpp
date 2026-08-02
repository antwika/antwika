#include "antwika/replay/ReplayReader.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <format>
#include <string>
#include <utility>

#include <antwika/log/Level.hpp>
#include <antwika/replay/DocumentDepth.hpp>
#include <antwika/replay/ReplayFormatError.hpp>
#include <antwika/replay/ReplayHeader.hpp>
#include <antwika/replay/ReplayJson.hpp>

#include "ReplayFormat.hpp"

namespace antwika::replay
{

    namespace
    {
        // Nothing is compared unless both sides say something.
        // A document with no canvas predates the field.
        // A caller with no canvas has claimed none.
        // Either way the canvas about to be used is the only one.
        void warnIfCanvasDiffers(
            const CanvasCheck &check, const ReplayDocument &document)
        {
            if (!check.canvas.has_value() || !document.canvas.has_value())
            {
                return;
            }
            if (*check.canvas == *document.canvas)
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
                    document.canvas->width,
                    document.canvas->height,
                    check.canvas->width,
                    check.canvas->height));
        }

        [[nodiscard]] bool isBlank(const std::string &line)
        {
            return line.find_first_not_of(" \t\r\n") == std::string::npos;
        }

        // The first JSON value in the stream.
        // It is a header or a whole version 1 document.
        // Which of the two it is is decided from what it holds.
        //
        // Not from the first non-space character.
        // That is sudoku::PuzzleFile's trick, and both shapes open '{'.
        // The member that held the whole event log tells them apart.
        // A header carries every other member a version 1 document did.
        // And never that one.
        [[nodiscard]] nlohmann::json readFirstValue(std::istream &in)
        {
            nlohmann::json first;
            try
            {
                in >> first;
            }
            catch (const nlohmann::json::parse_error &) // GCOVR_EXCL_LINE
            {
                throw ReplayFormatError(
                    "antwika::replay: not a valid replay stream (it does "
                    "not open with a JSON value)");
            }
            return first;
        } // GCOVR_EXCL_LINE

        // Every line after the header, as parsed JSON values.
        //
        // A record is one line ending in a newline.
        // That newline is what says the write got there whole.
        // So a last line without one that will not parse was torn off.
        // By the kill that ended the run, and it is dropped.
        // Appending as a run goes is pointless if the rest goes with it.
        // A line that will not parse anywhere else is a malformed file.
        // The reader says which line.
        [[nodiscard]] nlohmann::json readRecordLines(std::istream &in)
        {
            auto records = nlohmann::json::array();

            // The header was line 1.
            // The first read below returns what was left on it.
            // Which is nothing.
            std::size_t line = 1;
            std::string text;
            while (std::getline(in, text))
            {
                const std::size_t at = line;
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
                    if (in.eof())
                    {
                        break;
                    }

                    throw ReplayFormatError(std::format(
                        "antwika::replay: line {} of this replay is not "
                        "a JSON value",
                        at));
                }

                // Refused here, while the line number is still known.
                // Anything recursive over a deep value eats the stack.
                // See DocumentDepth.hpp.
                if (nestsTooDeep(records.back()))
                {
                    throw ReplayFormatError(std::format(
                        "antwika::replay: line {} of this replay nests "
                        "deeper than the format ever writes",
                        at));
                }
            }
            return records;
        } // GCOVR_EXCL_LINE

        // A version 1 document holds the run entire, so anything after
        // it is a second recording; returning after the first value
        // replayed two concatenated sessions as the first alone,
        // silently, where the line-oriented path refuses them loudly.
        void requireNothingAfter(std::istream &in)
        {
            std::string text;

            while (std::getline(in, text))
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
            std::istream &in, const MigrationChain &migrations)
        {
            const nlohmann::json first = readFirstValue(in);

            // Before the copy the whole-document branch would take.
            // Copying a deep value recurses; the parse above did not.
            // See DocumentDepth.hpp.
            if (nestsTooDeep(first))
            {
                throw ReplayFormatError(
                    "antwika::replay: the opening JSON value of this "
                    "replay nests deeper than the format ever writes");
            }

            // The key is a named local rather than a temporary.
            // A temporary std::string brings its own branches at -O0.
            const std::string events(detail::kLegacyEventsKey);
            if (first.is_object() && first.contains(events))
            {
                requireNothingAfter(in);

                return replayFromJson(first, migrations);
            }

            const ReplayHeader header =
                replayHeaderFromJson(first, migrations);

            ReplayDocument document;
            document.canvas = header.canvas;
            document.events = replayRecordsFromJson(
                readRecordLines(in), header.version, migrations);
            return document;
        } // GCOVR_EXCL_LINE
    } // namespace

    ReplayReader::ReplayReader(
        CanvasCheck check, MigrationChain migrations)
        : check(std::move(check)), migrations(std::move(migrations))
    {
    }

    std::vector<TickEvent> ReplayReader::read(std::istream &in) const
    {
        ReplayDocument document = readStream(in, migrations);
        warnIfCanvasDiffers(check, document);
        return std::move(document.events);
    }

} // namespace antwika::replay
