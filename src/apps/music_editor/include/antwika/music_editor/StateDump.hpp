#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string_view>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/music_editor/EditorState.hpp"
#include "antwika/music_editor/Playback.hpp"

namespace antwika::music_editor
{

    inline constexpr std::string_view kStateDumpMagic =
        "antwika-music-editor-state-dump";

    inline constexpr std::uint32_t kStateDumpVersion = 1;

    struct EditorDump final
    {
        EditorState editor;

        PlaybackMemory playback;

        [[nodiscard]] bool operator==(
            const EditorDump &other) const = default;
    };

    [[nodiscard]] antwika::replay::MigrationChain
    standardStateDumpMigrations();

    [[nodiscard]] nlohmann::json editorDumpToJson(
        const EditorDump &dump);

    [[nodiscard]] EditorDump editorDumpFromJson(
        const nlohmann::json &state);

}
