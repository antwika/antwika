#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <antwika/config/conformance/ConfigFileContractTest.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/sudoku/ConfigFile.hpp"

namespace antwika::config::conformance
{

    namespace
    {
        struct SudokuConfigTraits final
        {
            using Config = antwika::sudoku::SudokuConfig;

            static std::string_view magic()
            {
                return antwika::sudoku::kConfigMagic;
            }

            static std::uint32_t version()
            {
                return antwika::sudoku::kConfigFormatVersion;
            }

            static antwika::replay::MigrationChain migrations()
            {
                return antwika::sudoku::standardConfigMigrations();
            }

            static Config retuned()
            {
                Config config;
                config.solveStepBudget = 9;
                config.framePeriodMs = 60;
                return config;
            }

            static void expectEqual(
                const Config &decoded, const Config &expected)
            {
                EXPECT_EQ(decoded.solveStepBudget, expected.solveStepBudget);
                EXPECT_EQ(decoded.framePeriodMs, expected.framePeriodMs);
            }

            static const char *floorMember()
            {
                return "solveStepBudget";
            }

            static nlohmann::json toJson(const Config &config)
            {
                return antwika::sudoku::configToJson(config);
            }

            static Config fromJson(const nlohmann::json &document)
            {
                return antwika::sudoku::configFromJson(document);
            }

            static void write(const Config &config, std::ostream &out)
            {
                antwika::sudoku::writeConfig(config, out);
            }

            static Config read(std::istream &in)
            {
                return antwika::sudoku::readConfig(in);
            }

            static Config loadFileOrDefaults(const std::string &path)
            {
                return antwika::sudoku::loadConfigFileOrDefaults(path);
            }

            static std::string scratchPrefix()
            {
                return "antwika-sudoku-config";
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Sudoku, ConfigFileContractTest, SudokuConfigTraits);

}
