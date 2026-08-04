#pragma once

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/config/FileFormat.hpp"

namespace antwika::config
{

    /**
     * @brief An application's config loader, less the three functions
     * that are genuinely its own.
     *
     * Nine applications read a config file, and every one of them ended
     * in the same trailer: a static FileFormat built from a FormatSpec
     * of the same shape, an empty migration chain, and six free
     * functions forwarding to it.
     * Only the schema members, the encode and the decode ever differed
     * between them, so this is everything else, said once and
     * instantiated over what one application states.
     *
     * A Spec is that half as plain data -- its Config type, its kMagic
     * and kVersion, the kTitle and kWhatFailed a refusal names the
     * document by, and its kMembers, kEncode and kDecode function
     * pointers.
     * ANTWIKA_CONFIG_FILE writes one, and writes the six free
     * functions over it: a macro is the only thing that can give a
     * function in an application's namespace the name that
     * application's header has already promised, and every line it
     * emits is a call into this template, so what a macro contributes
     * is names and nothing else.
     *
     * The chain is built here rather than taken from the Spec because
     * all nine are empty and for the same reason: every member is
     * optional, so adding one is additive and a config format stays at
     * version 1 until a member changes meaning.
     * A format that does grow a migration outgrows this template and
     * states a FormatSpec of its own, which is what FileFormat is for.
     *
     * @tparam SpecT What one application states about its format.
     */
    template <typename SpecT>
    class AppConfigFile final
    {
    public:
        /** @brief The value this format reads and writes. */
        using Config = typename SpecT::Config;

        /** @brief The chain kind every persisted format migrates by. */
        using MigrationChain = replay::MigrationChain;

        AppConfigFile() = delete;

        /**
         * @brief Build the chain that brings an old document up.
         * @return A chain up to the format's current version; empty
         * today, and present anyway, since it is what refuses a
         * document from a newer build.
         */
        [[nodiscard]] static MigrationChain migrations()
        {
            // Every branch left on the excluded line is the allocator's.
            // The list is empty, so no heap branch is taken here.
            // What is left is the throw edge of building it.
            return MigrationChain({}, SpecT::kVersion); // GCOVR_EXCL_LINE
        }

        /**
         * @brief Encode a config as a document stating every member.
         * @param config The config to write.
         * @return The document.
         */
        [[nodiscard]] static nlohmann::json toJson(const Config &config)
        {
            return fileFormat().toJson(config);
        }

        /**
         * @brief Decode a config document.
         * @param document The parsed document.
         * @return The config it states, defaults filling the rest.
         * @throws ConfigFormatError If it is not this format, states a
         * version this build cannot reach the current one from, or
         * fails the schema.
         */
        [[nodiscard]] static Config fromJson(
            const nlohmann::json &document)
        {
            return fileFormat().fromJson(document);
        }

        /**
         * @brief Write a config to a stream.
         * @param config The config to write.
         * @param out Receives the document.
         */
        static void write(const Config &config, std::ostream &out)
        {
            fileFormat().write(config, out);
        }

        /**
         * @brief Read a config from a stream.
         * @param in Holds the document.
         * @return The config it holds.
         * @throws ConfigFormatError If the stream does not hold one.
         */
        [[nodiscard]] static Config read(std::istream &in)
        {
            return fileFormat().read(in);
        }

        /**
         * @brief Read the config an installation carries, if any.
         * @param path Where the file would be.
         * @return What it held, or the defaults when it is not there.
         * @throws ConfigFormatError If a file is there and is not one
         * of these.
         */
        [[nodiscard]] static Config loadFileOrDefaults(
            const std::string &path)
        {
            return fileFormat().loadFileOrDefaults(path);
        }

    private:
        [[nodiscard]] static const FileFormat<Config> &fileFormat()
        {
            using AppFormat = FileFormat<Config>;

            // The excluded closing line carries the static guard.
            // Its concurrency arms are unreachable one-threaded.
            // See docs/confirming-unreachable-branches.md.
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

} // namespace antwika::config

/**
 * @brief Define an application's config file functions over its three
 * own ones.
 *
 * Expands to the spec AppConfigFile is instantiated over and to the
 * six functions the application's header declares, each of them one
 * call into that template.
 * It is invoked inside the application's own namespace, and reads the
 * kConfigMagic and kConfigFormatVersion that namespace declares --
 * the two constants every one of these headers promises -- rather than
 * taking them as arguments, so a format cannot state one thing in its
 * header and another here.
 *
 * The two messages a refusal carries are built from the application's
 * name for the same reason: nine copies of "antwika::<app>: config
 * JSON failed schema validation: " were nine chances to word it
 * differently, and what a reader wants from one is which application
 * refused which file.
 *
 * @param appName The application's own name, as a string literal.
 * @param ConfigT The value this application's config file holds.
 * @param describeFn Adds this format's properties to the envelope
 * schema.
 * @param encodeFn States every member onto a stamped document.
 * @param decodeFn Decodes a migrated, validated document.
 */
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
