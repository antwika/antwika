#include "antwika/atlas_editor/EditorOptions.hpp"

#include <array>
#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>
#include <system_error>

#include <antwika/app/MaxTicks.hpp>

namespace antwika::atlas_editor
{

    namespace
    {
        constexpr std::string_view kImageFlag = "--image";
        constexpr std::string_view kOutFlag = "--out";
        constexpr std::string_view kSheetFlag = "--sheet";
        constexpr std::string_view kTileFlag = "--tile";

        constexpr std::array kFlags{
            antwika::cli::FlagSpec{
                .name = kImageFlag,
                .valueName = "<path>",
                .help = "Open this PNG; without it the session starts "
                        "on a blank sheet."},
            antwika::cli::FlagSpec{
                .name = kOutFlag,
                .valueName = "<path>",
                .help = "Write this PNG when Save is pressed; without "
                        "it nothing can be saved."},
            antwika::cli::FlagSpec{
                .name = kSheetFlag,
                .valueName = "<w>x<h>",
                .help = "How big a blank sheet to open (default "
                        "512x768)."},
            antwika::cli::FlagSpec{
                .name = kTileFlag,
                .valueName = "<w>x<h>",
                .help = "What the grid overlay's slots are (default "
                        "64x96)."},
            antwika::cli::FlagSpec{
                .name = antwika::app::kMaxTicksFlag,
                .valueName = "<n>",
                .help = "Give up after <n> ticks (default 90000; 0 "
                        "runs until the window is closed)."}};

        [[nodiscard]] std::optional<std::uint64_t> parseNumber(
            const std::string_view text)
        {
            std::uint64_t value = 0;
            const auto read = std::from_chars(
                text.data(), text.data() + text.size(), value);

            if (read.ec != std::errc{}
                || read.ptr != text.data() + text.size())
            {
                return std::nullopt;
            }

            return value;
        }
    }

    std::optional<Size> parseSize(const std::string_view text)
    {
        const auto cross = text.find('x');
        if (cross == std::string_view::npos)
        {
            return std::nullopt;
        }

        const auto width = parseNumber(text.substr(0, cross));
        const auto height = parseNumber(text.substr(cross + 1));

        constexpr std::uint64_t kMostExtent =
            std::numeric_limits<std::uint32_t>::max();

        if (!width.has_value() || !height.has_value() || *width == 0
            || *height == 0 || *width > kMostExtent
            || *height > kMostExtent)
        {
            return std::nullopt;
        }

        return Size{
            .width = static_cast<std::uint32_t>(*width),
            .height = static_cast<std::uint32_t>(*height)};
    }

    std::span<const antwika::cli::FlagSpec> editorFlags()
    {
        return kFlags;
    }

    EditorOptions editorOptionsFrom(
        const antwika::cli::CommandLine &parsed)
    {
        EditorOptions options;

        options.imagePath = parsed.value(kImageFlag);
        options.outPath = parsed.value(kOutFlag);

        if (const auto sheet = parsed.value(kSheetFlag); sheet)
        {
            options.sheet = parseSize(*sheet).value_or(options.sheet);
        }

        if (const auto tile = parsed.value(kTileFlag); tile)
        {
            const auto size = parseSize(*tile);

            if (size.has_value())
            {
                options.tile = TileGrid{
                    .width = size->width, .height = size->height};
            }
        }

        options.maxTicks = antwika::app::maxTicksOf(
            parsed.value(antwika::app::kMaxTicksFlag),
            options.maxTicks);

        return options;
    } // GCOVR_EXCL_LINE

}
