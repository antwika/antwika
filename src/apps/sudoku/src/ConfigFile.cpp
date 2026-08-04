#include "antwika/sudoku/ConfigFile.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include <antwika/config/ConfigDocument.hpp>
#include <antwika/config/FileFormat.hpp>
#include <antwika/config/Format.hpp>

namespace antwika::sudoku
{

    namespace
    {
        using antwika::config::FileFormat;
        using antwika::config::FormatSpec;
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

        const FileFormat<SudokuConfig> &fileFormat()
        {
            using AppFormat = FileFormat<SudokuConfig>;

            // The excluded closing line carries the static guard.
            // Its concurrency arms are unreachable one-threaded.
            // See docs/confirming-unreachable-branches.md.
            static const AppFormat format(
                FormatSpec<SudokuConfig>{
                    .format =
                        {.magic = kConfigMagic,
                         .version = kConfigFormatVersion},
                    .title = "antwika sudoku config document",
                    .whatFailed =
                        "antwika::sudoku: config JSON failed schema "
                        "validation: ",
                    .members = describeMembers,
                    .encode = encodeMembers,
                    .decode = decodeMembers,
                    .migrations = standardConfigMigrations}); // GCOVR_EXCL_LINE
            return format;
        }
    } // namespace

    MigrationChain standardConfigMigrations()
    {
        // Every branch left on the excluded line is the allocator's.
        // The list is empty, so no heap branch is taken here.
        // What is left is the throw edge of building it.
        return MigrationChain({}, kConfigFormatVersion); // GCOVR_EXCL_LINE
    }

    nlohmann::json configToJson(const SudokuConfig &config)
    {
        return fileFormat().toJson(config);
    }

    SudokuConfig configFromJson(const nlohmann::json &document)
    {
        return fileFormat().fromJson(document);
    }

    void writeConfig(const SudokuConfig &config, std::ostream &out)
    {
        fileFormat().write(config, out);
    }

    SudokuConfig readConfig(std::istream &in)
    {
        return fileFormat().read(in);
    }

    SudokuConfig loadConfigFileOrDefaults(const std::string &path)
    {
        return fileFormat().loadFileOrDefaults(path);
    }

} // namespace antwika::sudoku
