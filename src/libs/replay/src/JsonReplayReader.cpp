#include "antwika/replay/JsonReplayReader.hpp"

#include <nlohmann/json.hpp>

#include <antwika/replay/ReplayFormatError.hpp>
#include <antwika/replay/ReplayJson.hpp>

namespace antwika::replay
{

    std::vector<TimedEvent> JsonReplayReader::read(std::istream &in) const
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
        return replayFromJson(parsed);
    }

} // namespace antwika::replay
