#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <antwika/config/conformance/ConfigFileContractTest.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/map_editor/ConfigFile.hpp"

namespace antwika::config::conformance
{

    namespace
    {
        struct MapEditorConfigTraits final
        {
            using Config = antwika::map_editor::MapEditorConfig;

            static std::string_view magic()
            {
                return antwika::map_editor::kConfigMagic;
            }

            static std::uint32_t version()
            {
                return antwika::map_editor::kConfigFormatVersion;
            }

            static antwika::replay::MigrationChain migrations()
            {
                return antwika::map_editor::standardConfigMigrations();
            }

            static Config retuned()
            {
                Config config;
                config.uiScale = 4;
                config.fullscreen = true;
                config.keys = {{"undo", "z"}, {"redo", "y"}};

                return config;
            }

            static void expectEqual(
                const Config &decoded, const Config &expected)
            {
                EXPECT_EQ(decoded.uiScale, expected.uiScale);
                EXPECT_EQ(decoded.fullscreen, expected.fullscreen);
                EXPECT_EQ(decoded.keys, expected.keys);
            }

            static const char *floorMember()
            {
                return "uiScale";
            }

            static nlohmann::json toJson(const Config &config)
            {
                return antwika::map_editor::configToJson(config);
            }

            static Config fromJson(const nlohmann::json &document)
            {
                return antwika::map_editor::configFromJson(document);
            }

            static void write(const Config &config, std::ostream &out)
            {
                antwika::map_editor::writeConfig(config, out);
            }

            static Config read(std::istream &in)
            {
                return antwika::map_editor::readConfig(in);
            }

            static Config loadFileOrDefaults(const std::string &path)
            {
                return antwika::map_editor::loadConfigFileOrDefaults(
                    path);
            }

            static std::string scratchPrefix()
            {
                return "antwika-map_editor-config";
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        MapEditor, ConfigFileContractTest, MapEditorConfigTraits);

}
