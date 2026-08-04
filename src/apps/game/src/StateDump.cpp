#include "antwika/game/StateDump.hpp"

#include <array>
#include <cstddef>
#include <utility>

#include <nlohmann/json-schema.hpp>

#include <antwika/config/FileFormat.hpp>
#include <antwika/config/Format.hpp>

#include "antwika/game/SaveFormatError.hpp"

namespace antwika::game
{

    namespace
    {
        using antwika::config::FileFormat;
        using antwika::config::FormatSpec;

        using DumpFormat = FileFormat<StateDump, SaveFormatError>;

        // The names a dump document holds, one per tool.
        // Persisted, so they may not change once written.
        // The same rule buildingKindName() is held to.
        constexpr std::array<std::string_view, kBuildToolCount>
            kToolNames{
                "road",
                "house",
                "farm",
                "clay_pit",
                "workshop",
                "storage",
                "market",
                "well",
                "doctor",
                "fire_station",
                "engineer_post",
                "raze"};

        // The names a dump document holds, one per view.
        constexpr std::array<std::string_view, kMapViewCount> kViewNames{
            "normal",
            "desirability",
            "food",
            "water",
            "medicine",
            "fire",
            "damage"};

        [[nodiscard]] std::string_view toolName(BuildTool tool) noexcept
        {
            return kToolNames[buildToolIndex(tool) % kBuildToolCount];
        }

        [[nodiscard]] std::optional<BuildTool> toolFromName(
            std::string_view name) noexcept
        {
            for (std::size_t index = 0; index < kBuildToolCount; ++index)
            {
                if (kToolNames[index] == name)
                {
                    return static_cast<BuildTool>(index);
                }
            }

            return std::nullopt;
        }

        [[nodiscard]] std::string_view viewName(MapView view) noexcept
        {
            return kViewNames[mapViewIndex(view) % kMapViewCount];
        }

        [[nodiscard]] std::optional<MapView> viewFromName(
            std::string_view name) noexcept
        {
            for (std::size_t index = 0; index < kMapViewCount; ++index)
            {
                if (kViewNames[index] == name)
                {
                    return static_cast<MapView>(index);
                }
            }

            return std::nullopt;
        }

        void describeMembers(nlohmann::json &schema)
        {
            // The save is a whole versioned document of its own.
            // saveGameFromJson() migrates and validates it itself.
            // The envelope's schema only says that one is there.
            schema["required"] = {
                "magic",
                "save",
                "paused",
                "mapView",
                "locale",
                "console"}; // GCOVR_EXCL_LINE
            schema["properties"]["save"]["type"] = "object";
            schema["properties"]["paused"]["type"] = "boolean";
            schema["properties"]["tool"]["type"] = "string";
            schema["properties"]["mapView"]["type"] = "string";
            schema["properties"]["locale"]["type"] = "string";
            schema["properties"]["console"]["type"] = "array";
            schema["properties"]["console"]["items"]["type"] = "string";
        }

        void encodeMembers(const StateDump &dump, nlohmann::json &encoded)
        {
            encoded["save"] = saveGameToJson(dump.save);
            encoded["paused"] = dump.paused;

            // Absent means the palette was down.
            // A member for it would be a name for no tool.
            if (dump.tool.has_value())
            {
                encoded["tool"] = std::string(toolName(*dump.tool));
            }

            encoded["mapView"] = std::string(viewName(dump.view));
            encoded["locale"] =
                std::string(antwika::i18n::tagOf(dump.locale));
            encoded["console"] = dump.console;
        }

        StateDump decodeMembers(const nlohmann::json &document)
        {
            StateDump dump;

            dump.save = saveGameFromJson(document.at("save"));
            dump.paused = document.at("paused").get<bool>();

            if (document.contains("tool"))
            {
                const auto named =
                    document.at("tool").get<std::string>();
                const auto tool = toolFromName(named);

                if (!tool.has_value())
                {
                    throw SaveFormatError(
                        "antwika::game: dump names a tool this build "
                        "does not know: "
                        + named);
                }

                dump.tool = tool;
            }
            else
            {
                dump.tool = std::nullopt;
            }

            const auto viewNamed =
                document.at("mapView").get<std::string>();
            const auto view = viewFromName(viewNamed);

            if (!view.has_value())
            {
                throw SaveFormatError(
                    "antwika::game: dump names a map view this build "
                    "does not know: "
                    + viewNamed);
            }

            dump.view = *view;

            const auto tag = document.at("locale").get<std::string>();
            const auto locale = antwika::i18n::localeFromTag(tag);

            if (!locale.has_value())
            {
                throw SaveFormatError(
                    "antwika::game: dump names a language this build "
                    "has no catalogue for: "
                    + tag);
            }

            dump.locale = *locale;
            dump.console =
                document.at("console").get<std::vector<std::string>>();

            return dump;
        }

        const DumpFormat &dumpFormat()
        {
            // The excluded closing line carries the static guard.
            // Its concurrency arms are unreachable one-threaded.
            // See docs/confirming-unreachable-branches.md.
            static const DumpFormat format(
                FormatSpec<StateDump>{
                    .format =
                        {.magic = kStateDumpMagic,
                         .version = kStateDumpVersion},
                    .title = "antwika game state dump document",
                    .whatFailed = "antwika::game: state dump JSON failed "
                                  "schema validation: ",
                    .members = describeMembers,
                    .encode = encodeMembers,
                    .decode = decodeMembers,
                    .migrations =
                        standardStateDumpMigrations}); // GCOVR_EXCL_LINE
            return format;
        }
    } // namespace

    antwika::replay::MigrationChain standardStateDumpMigrations()
    {
        return antwika::replay::MigrationChain(
            {}, kStateDumpVersion); // GCOVR_EXCL_LINE
    }

    nlohmann::json stateDumpToJson(const StateDump &dump)
    {
        return dumpFormat().toJson(dump);
    }

    StateDump stateDumpFromJson(const nlohmann::json &document)
    {
        return dumpFormat().fromJson(document);
    }

} // namespace antwika::game
