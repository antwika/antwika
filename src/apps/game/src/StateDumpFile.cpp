#include "antwika/game/StateDumpFile.hpp"

#include <nlohmann/json.hpp>

#include <antwika/config/ConfigDocument.hpp>
#include <antwika/io/File.hpp>

#include "antwika/game/SaveFormatError.hpp"

namespace antwika::game
{

    void writeStateDump(const StateDump &dump, std::ostream &out)
    {
        antwika::config::writeConfig(stateDumpToJson(dump), out);
    }

    StateDump readStateDump(std::istream &in)
    {
        // The parse is antwika::config's.
        // So a malformed document reads the same way everywhere.
        return stateDumpFromJson(
            antwika::config::parseAs<SaveFormatError>(in));
    }

    void stateDumpFile(const StateDump &dump, const std::string &path)
    {
        // The open, the flush and the write refusal are antwika::io's.
        // That discipline is stated once, over there.
        io::writeFileAs<SaveFormatError>(
            path, "a state dump", [&dump](std::ostream &out) {
                writeStateDump(dump, out);
            });
    }

    StateDump loadStateDump(const std::string &path)
    {
        // A file that is not there is not a malformed document.
        // Unchecked it reaches the parser as an empty stream.
        // Which reports a missing dump as invalid JSON.
        auto file = io::openToReadAs<SaveFormatError>(path, "a state dump");

        return readStateDump(file);
    }

} // namespace antwika::game
