#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <optional>
#include <string_view>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/atlas_editor/CanvasView.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/Pixel.hpp"
#include "antwika/atlas_editor/Preview.hpp"
#include "antwika/atlas_editor/Selection.hpp"
#include "antwika/atlas_editor/Tool.hpp"

namespace antwika::atlas_editor
{

    inline constexpr std::string_view kStateDumpMagic =
        "antwika-atlas-editor-state-dump";

    inline constexpr std::uint32_t kStateDumpVersion = 1;

    struct DumpedImage final
    {
        antwika::gfx::Size size{};

        std::uint64_t fingerprint = 0;

        [[nodiscard]] bool operator==(const DumpedImage &other) const =
            default;
    };

    struct EditorStateDump final
    {
        DumpedImage sheet{};

        std::uint64_t sheetRevision = 0;

        std::optional<DumpedImage> clipboard;

        CanvasView view{};

        Tool tool = Tool::Paint;

        antwika::gfx::Color paint{};

        std::optional<std::size_t> swatch;

        bool showGrid = true;

        bool showGuides = true;

        bool showPivot = false;

        bool showPointerBorder = true;

        std::optional<Pixel> under;

        std::optional<Selection> marked;

        std::optional<Gesture> gesture;

        PreviewPane preview{};

        std::uint64_t changes = 0;

        std::uint64_t stepped = 0;

        std::uint32_t written = 0;

        std::uint32_t read = 0;

        std::uint64_t savedRevision = 0;

        [[nodiscard]] bool operator==(
            const EditorStateDump &other) const = default;
    };

    [[nodiscard]] antwika::replay::MigrationChain
    standardStateDumpMigrations();

    [[nodiscard]] nlohmann::json stateDumpToJson(
        const EditorStateDump &dump);

    [[nodiscard]] EditorStateDump stateDumpFromJson(
        const nlohmann::json &state);

}
