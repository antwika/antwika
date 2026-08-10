#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/config/FileFormat.hpp"

namespace antwika::config
{

    template <typename SpecT>
    class AppConfigFile final
    {
    public:
        using Config = typename SpecT::Config;

        using MigrationChain = replay::MigrationChain;

        AppConfigFile() = delete;

        [[nodiscard]] static MigrationChain migrations()
        {
            return MigrationChain({}, SpecT::kVersion); // GCOVR_EXCL_LINE
        }

        [[nodiscard]] static nlohmann::json toJson(const Config &config)
        {
            return fileFormat().toJson(config);
        }

        [[nodiscard]] static Config fromJson(
            const nlohmann::json &document)
        {
            return fileFormat().fromJson(document);
        }

        static void write(const Config &config, std::ostream &out)
        {
            fileFormat().write(config, out);
        }

        [[nodiscard]] static Config read(std::istream &in)
        {
            return fileFormat().read(in);
        }

        [[nodiscard]] static Config loadFileOrDefaults(
            const std::string &path)
        {
            return fileFormat().loadFileOrDefaults(path);
        }

    private:
        [[nodiscard]] static const FileFormat<Config> &fileFormat()
        {
            using AppFormat = FileFormat<Config>;

            static const AppFormat format(spec()); // GCOVR_EXCL_LINE
            return format;
        }

        [[nodiscard]] static FormatSpec<Config> spec()
        {
            return {
                .format =
                    {.magic = SpecT::kMagic,
                     .version = SpecT::kVersion},
                .title = SpecT::kTitle,
                .whatFailed = SpecT::kWhatFailed,
                .members = SpecT::kMembers,
                .encode = SpecT::kEncode,
                .decode = SpecT::kDecode,
                .migrations = &AppConfigFile::migrations};
        }
    };

}

#define ANTWIKA_CONFIG_FILE(                                          \
    appName, ConfigT, describeFn, encodeFn, decodeFn)                 \
    namespace                                                         \
    {                                                                 \
        struct ConfigSpec final                                       \
        {                                                             \
            using Config = ConfigT;                                   \
                                                                      \
            static constexpr std::string_view kMagic =                \
                kConfigMagic;                                         \
            static constexpr std::uint32_t kVersion =                 \
                kConfigFormatVersion;                                 \
            static constexpr std::string_view kTitle =                \
                "antwika " appName " config document";                \
            static constexpr std::string_view kWhatFailed =           \
                "antwika::" appName ": config JSON failed "           \
                "schema validation: ";                                \
            static constexpr auto kMembers = describeFn;              \
            static constexpr auto kEncode = encodeFn;                 \
            static constexpr auto kDecode = decodeFn;                 \
        };                                                            \
                                                                      \
        using AppConfigFileOf =                                       \
            ::antwika::config::AppConfigFile<ConfigSpec>;             \
    }                                                                 \
                                                                      \
    ::antwika::replay::MigrationChain standardConfigMigrations()      \
    {                                                                 \
        return AppConfigFileOf::migrations();                         \
    }                                                                 \
                                                                      \
    nlohmann::json configToJson(const ConfigT &config)                \
    {                                                                 \
        return AppConfigFileOf::toJson(config);                       \
    }                                                                 \
                                                                      \
    ConfigT configFromJson(const nlohmann::json &document)            \
    {                                                                 \
        return AppConfigFileOf::fromJson(document);                   \
    }                                                                 \
                                                                      \
    void writeConfig(const ConfigT &config, std::ostream &out)        \
    {                                                                 \
        AppConfigFileOf::write(config, out);                          \
    }                                                                 \
                                                                      \
    ConfigT readConfig(std::istream &in)                              \
    {                                                                 \
        return AppConfigFileOf::read(in);                             \
    }                                                                 \
                                                                      \
    ConfigT loadConfigFileOrDefaults(const std::string &path)         \
    {                                                                 \
        return AppConfigFileOf::loadFileOrDefaults(path);             \
    }
