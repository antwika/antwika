#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <antwika/config/conformance/ConfigFileContractTest.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/atlas_editor/ConfigFile.hpp"

namespace antwika::config::conformance
{

    namespace
    {
        struct AtlasEditorConfigTraits final
        {
            using Config = antwika::atlas_editor::AtlasEditorConfig;

            static std::string_view magic()
            {
                return antwika::atlas_editor::kConfigMagic;
            }

            static std::uint32_t version()
            {
                return antwika::atlas_editor::kConfigFormatVersion;
            }

            static antwika::replay::MigrationChain migrations()
            {
                return antwika::atlas_editor::standardConfigMigrations();
            }

            static Config retuned()
            {
                Config config;
                config.framePeriodMs = 60;
                return config;
            }

            static void expectEqual(
                const Config &decoded, const Config &expected)
            {
                EXPECT_EQ(decoded.framePeriodMs, expected.framePeriodMs);
            }

            static const char *floorMember()
            {
                return "framePeriodMs";
            }

            static nlohmann::json toJson(const Config &config)
            {
                return antwika::atlas_editor::configToJson(config);
            }

            static Config fromJson(const nlohmann::json &document)
            {
                return antwika::atlas_editor::configFromJson(document);
            }

            static void write(const Config &config, std::ostream &out)
            {
                antwika::atlas_editor::writeConfig(config, out);
            }

            static Config read(std::istream &in)
            {
                return antwika::atlas_editor::readConfig(in);
            }

            static Config loadFileOrDefaults(const std::string &path)
            {
                return antwika::atlas_editor::loadConfigFileOrDefaults(path);
            }

            static std::string scratchPrefix()
            {
                return "antwika-atlas_editor-config";
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        AtlasEditor, ConfigFileContractTest, AtlasEditorConfigTraits);

}
