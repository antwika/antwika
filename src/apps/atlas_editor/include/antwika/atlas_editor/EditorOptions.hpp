#pragma once

#include <optional>
#include <span>
#include <string>
#include <string_view>

#include <antwika/cli/CommandLine.hpp>
#include <antwika/cli/FlagSpec.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/atlas_editor/AtlasEditor.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Size;

    inline constexpr antwika::time::Tick kDefaultMaxTicks = 90000;

    struct EditorOptions final
    {
        std::optional<std::string> imagePath{};

        std::optional<std::string> outPath{};

        Size sheet = kDefaultSheetSize;

        TileGrid tile{};

        std::optional<antwika::time::Tick> maxTicks{kDefaultMaxTicks};

        [[nodiscard]] bool operator==(const EditorOptions &other) const =
            default;
    };

    [[nodiscard]] std::optional<Size> parseSize(std::string_view text);

    [[nodiscard]] std::span<const antwika::cli::FlagSpec>
    editorFlags();

    [[nodiscard]] EditorOptions editorOptionsFrom(
        const antwika::cli::CommandLine &parsed);

}
