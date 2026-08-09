#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <antwika/config/conformance/ConfigFileContractTest.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/music_editor/ConfigFile.hpp"

namespace antwika::config::conformance
{

    namespace
    {
        struct MusicEditorConfigTraits final
        {
            using Config = antwika::music_editor::MusicEditorConfig;

            static std::string_view magic()
            {
                return antwika::music_editor::kConfigMagic;
            }

            static std::uint32_t version()
            {
                return antwika::music_editor::kConfigFormatVersion;
            }

            static antwika::replay::MigrationChain migrations()
            {
                return antwika::music_editor::standardConfigMigrations();
            }

            static Config retuned()
            {
                Config config;
                config.tickIntervalMs = 50;
                return config;
            }

            static void expectEqual(
                const Config &decoded, const Config &expected)
            {
                EXPECT_EQ(decoded.tickIntervalMs, expected.tickIntervalMs);
            }

            static const char *floorMember()
            {
                return "tickIntervalMs";
            }

            static nlohmann::json toJson(const Config &config)
            {
                return antwika::music_editor::configToJson(config);
            }

            static Config fromJson(const nlohmann::json &document)
            {
                return antwika::music_editor::configFromJson(document);
            }

            static void write(const Config &config, std::ostream &out)
            {
                antwika::music_editor::writeConfig(config, out);
            }

            static Config read(std::istream &in)
            {
                return antwika::music_editor::readConfig(in);
            }

            static Config loadFileOrDefaults(const std::string &path)
            {
                return antwika::music_editor::loadConfigFileOrDefaults(path);
            }

            static std::string scratchPrefix()
            {
                return "antwika-music_editor-config";
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        MusicEditor, ConfigFileContractTest, MusicEditorConfigTraits);

}
