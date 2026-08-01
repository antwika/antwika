#include "antwika/replay/ReplayReader.hpp"

#include <nlohmann/json.hpp>

#include <format>
#include <utility>

#include <antwika/log/Level.hpp>
#include <antwika/replay/ReplayFormatError.hpp>
#include <antwika/replay/ReplayJson.hpp>

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
    } // namespace

    ReplayReader::ReplayReader(
        CanvasCheck check, MigrationChain migrations)
        : check(std::move(check)), migrations(std::move(migrations))
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

        ReplayDocument document = replayFromJson(parsed, migrations);
        warnIfCanvasDiffers(check, document);
        return std::move(document.events);
    }

} // namespace antwika::replay
