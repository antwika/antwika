#include "antwika/sudoku/ConfigFile.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include <antwika/config/AppConfigFile.hpp>
#include <antwika/config/ConfigDocument.hpp>

namespace antwika::sudoku
{

    namespace
    {
        using antwika::config::memberOr;
        using antwika::config::wholeShape;

        void describeMembers(nlohmann::json &schema)
        {
            schema["properties"]["solveStepBudget"] =
                wholeShape(1, std::numeric_limits<std::int64_t>::max());
            schema["properties"]["framePeriodMs"] =
                wholeShape(1, std::numeric_limits<std::int32_t>::max());
        }

        void encodeMembers(const SudokuConfig &config, nlohmann::json &out)
        {
            out["solveStepBudget"] = config.solveStepBudget;
            out["framePeriodMs"] = config.framePeriodMs;
        }

        SudokuConfig decodeMembers(const nlohmann::json &document)
        {
            SudokuConfig config;
            config.solveStepBudget =
                memberOr(document, "solveStepBudget", config.solveStepBudget);
            config.framePeriodMs =
                memberOr(document, "framePeriodMs", config.framePeriodMs);
            return config;
        }
    } // namespace

    ANTWIKA_CONFIG_FILE(
        "sudoku",
        SudokuConfig,
        describeMembers,
        encodeMembers,
        decodeMembers)

} // namespace antwika::sudoku
