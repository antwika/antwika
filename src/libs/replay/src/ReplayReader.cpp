#include "antwika/replay/ReplayReader.hpp"

#include <nlohmann/json.hpp>

#include <format>
#include <string>
#include <utility>

#include <antwika/log/Level.hpp>
#include <antwika/replay/ReplayFormatError.hpp>
#include <antwika/replay/ReplayJson.hpp>
#include <antwika/replay/SchemaVersion.hpp>
#include <antwika/replay/SchemaVersionError.hpp>

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

        // The version is read before anything else is looked at.
        // A document from a newer build may parse and may validate.
        // It can still mean something else entirely.
        // So "which revision is this" has to be answered first.
        void bringToCurrentVersion(nlohmann::json &parsed)
        {
            if (!parsed.is_object())
            {
                return; // The schema refuses it, and says why better.
            }

            const auto version = documentVersion(parsed);
            if (version != kReplayDocumentVersion)
            {
                throw SchemaVersionError(std::format(
                    "antwika::replay: this replay states schema version "
                    "{}, and this build reads version {}",
                    version,
                    kReplayDocumentVersion));
            }

            // A document with no version member is version 1.
            // Stamping it here is what lets the schema require one.
            parsed[std::string(kSchemaVersionKey)] =
                kReplayDocumentVersion;
        }
    } // namespace

    ReplayReader::ReplayReader(CanvasCheck check) noexcept
        : check(std::move(check))
    {
    }

    std::vector<TickEvent> ReplayReader::read(std::istream &in) const
    {
        nlohmann::json parsed;
        try
        {
            in >> parsed;
        }
        catch (const nlohmann::json::parse_error &) // GCOVR_EXCL_LINE
        {
            throw ReplayFormatError(
                "antwika::replay: not a valid replay stream (not valid "
                "JSON)");
        }

        bringToCurrentVersion(parsed);

        ReplayDocument document = replayFromJson(parsed);
        warnIfCanvasDiffers(check, document);
        return std::move(document.events);
    }

} // namespace antwika::replay
