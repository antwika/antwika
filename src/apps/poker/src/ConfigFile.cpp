#include "antwika/poker/ConfigFile.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include <antwika/config/ConfigDocument.hpp>
#include <antwika/config/FileFormat.hpp>
#include <antwika/config/Format.hpp>
#include <antwika/config/ConfigFormatError.hpp>

namespace antwika::poker
{

    namespace
    {
        using antwika::config::FileFormat;
        using antwika::config::FormatSpec;
        using antwika::config::memberOr;
        using antwika::config::wholeShape;

        // A blind or a buy-in of nothing is no cash game at all.
        // So the floor is one chip, stated beside the parse.
        nlohmann::json chipsShape()
        {
            return wholeShape(
                1, std::numeric_limits<std::int64_t>::max());
        }

        void describeMembers(nlohmann::json &schema)
        {
            schema["properties"]["smallBlind"] = chipsShape();
            schema["properties"]["bigBlind"] = chipsShape();
            schema["properties"]["minimumBuyIn"] = chipsShape();

            // One rating per category, weakest first, 0..100.
            auto &strengths = schema["properties"]["handStrengths"];
            strengths["type"] = "array";
            strengths["items"] = wholeShape(0, 100);
            strengths["minItems"] = kHandCategoryCount;
            strengths["maxItems"] = kHandCategoryCount;
        }

        void encodeMembers(const RoomConfig &config, nlohmann::json &out)
        {
            out["smallBlind"] = config.blinds.small;
            out["bigBlind"] = config.blinds.big;
            out["minimumBuyIn"] = config.minimumBuyIn;

            for (const auto strength : config.handStrengths)
            {
                out["handStrengths"].push_back(strength);
            }
        }

        RoomConfig decodeMembers(const nlohmann::json &document)
        {
            RoomConfig config;
            config.blinds.small =
                memberOr(document, "smallBlind", config.blinds.small);
            config.blinds.big =
                memberOr(document, "bigBlind", config.blinds.big);
            config.minimumBuyIn =
                memberOr(document, "minimumBuyIn", config.minimumBuyIn);

            if (document.contains("handStrengths"))
            {
                const auto &strengths = document.at("handStrengths");

                for (std::size_t index = 0;
                     index < kHandCategoryCount;
                     ++index)
                {
                    config.handStrengths[index] =
                        strengths.at(index).get<unsigned>();
                }
            }

            // The schema checks each number alone.
            // These two rules are between numbers.
            // So they are refused beside the decode.
            if (config.blinds.big < config.blinds.small)
            {
                throw antwika::config::ConfigFormatError(
                    "antwika::poker: config states a big blind "
                    "smaller than the small blind");
            }

            // Weakest first is the table's whole meaning.
            // A straight rated under a pair was written backwards.
            for (std::size_t index = 1; index < kHandCategoryCount;
                 ++index)
            {
                if (config.handStrengths[index]
                    < config.handStrengths[index - 1])
                {
                    throw antwika::config::ConfigFormatError(
                        "antwika::poker: config rates a stronger "
                        "hand category under a weaker one");
                }
            }

            if (config.minimumBuyIn < config.blinds.big)
            {
                throw antwika::config::ConfigFormatError(
                    "antwika::poker: config states a minimum "
                    "buy-in smaller than the big blind");
            }
            return config;
        }

        const FileFormat<RoomConfig> &fileFormat()
        {
            using AppFormat = FileFormat<RoomConfig>;

            // The excluded closing line carries the static guard.
            // Its concurrency arms are unreachable one-threaded.
            // See docs/confirming-unreachable-branches.md.
            static const AppFormat format(
                FormatSpec<RoomConfig>{
                    .format =
                        {.magic = kConfigMagic,
                         .version = kConfigFormatVersion},
                    .title = "antwika poker config document",
                    .whatFailed =
                        "antwika::poker: config JSON failed schema "
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

    nlohmann::json configToJson(const RoomConfig &config)
    {
        return fileFormat().toJson(config);
    }

    RoomConfig configFromJson(const nlohmann::json &document)
    {
        return fileFormat().fromJson(document);
    }

    void writeConfig(const RoomConfig &config, std::ostream &out)
    {
        fileFormat().write(config, out);
    }

    RoomConfig readConfig(std::istream &in)
    {
        return fileFormat().read(in);
    }

    RoomConfig loadConfigFileOrDefaults(const std::string &path)
    {
        return fileFormat().loadFileOrDefaults(path);
    }

} // namespace antwika::poker
