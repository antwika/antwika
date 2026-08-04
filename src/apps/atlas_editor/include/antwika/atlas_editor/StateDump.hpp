#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include <nlohmann/json.hpp>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/atlas_editor/CanvasView.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/Pixel.hpp"
#include "antwika/atlas_editor/Selection.hpp"
#include "antwika/atlas_editor/Tool.hpp"

namespace antwika::atlas_editor
{

    /** @brief What names this application's dump file as its own. */
    inline constexpr std::string_view kStateDumpMagic =
        "antwika-atlas-editor-state-dump";

    /** @brief The dump document version this build writes and reads. */
    inline constexpr std::uint32_t kStateDumpVersion = 1;

    /**
     * @brief A bitmap as the dump document carries it: a shape and a
     * fingerprint, never the pixels.
     *
     * The pixels live in a PNG beside the JSON -- a sheet inline would
     * be megabytes of numbers -- and the fingerprint is what binds the
     * two files back together on a load.
     */
    struct DumpedImage
    {
        /** @brief How big the bitmap is. */
        antwika::gfx::Size size{};

        /** @brief fingerprintOf() over the bitmap, checked on load. */
        std::uint64_t fingerprint = 0;

        /**
         * @brief Compare two dumped images.
         * @param other The image to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const DumpedImage &other) const =
            default;
    };

    /**
     * @brief One session's whole state, as the dump document holds it.
     *
     * Everything EditorState is, member for member, except the status
     * message -- transient, the last thing said rather than part of
     * what the session is, so it is deliberately dropped -- and the
     * two bitmaps, carried as side PNGs and named here by fingerprint.
     */
    struct EditorStateDump
    {
        /** @brief The sheet's shape and fingerprint. */
        DumpedImage sheet{};

        /** @brief The sheet's change count when the dump was taken. */
        std::uint64_t sheetRevision = 0;

        /** @brief The clipboard's shape and fingerprint, if any. */
        std::optional<DumpedImage> clipboard;

        /** @brief Where the sheet sat and how far in it was zoomed. */
        CanvasView view{};

        /** @brief The selected tool. */
        Tool tool = Tool::Paint;

        /** @brief The colour Paint puts down. */
        antwika::gfx::Color paint{};

        /** @brief Which swatch was selected, if any. */
        std::optional<std::size_t> swatch;

        /** @brief Whether the slot grid was drawn. */
        bool showGrid = true;

        /** @brief Whether the sprite guides were drawn. */
        bool showGuides = true;

        /** @brief The image pixel the pointer was last over. */
        std::optional<Pixel> under;

        /** @brief The rectangle marked out, if any. */
        std::optional<Selection> marked;

        /** @brief The drag in progress, if any. */
        std::optional<Gesture> gesture;

        /** @brief How many pixels the session had changed. */
        std::uint64_t changes = 0;

        /** @brief How many ticks the session had run. */
        std::uint64_t stepped = 0;

        /** @brief How many times the sheet had been written out. */
        std::uint32_t written = 0;

        /** @brief How many times a sheet had been read in. */
        std::uint32_t read = 0;

        /** @brief The revision the last save was taken at. */
        std::uint64_t savedRevision = 0;

        /**
         * @brief Compare two dumps.
         * @param other The dump to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(
            const EditorStateDump &other) const = default;
    };

    /**
     * @brief Build the migration chain a dump document is read up
     * through.
     *
     * Empty at version one, exactly as every persisted format here
     * starts: the chain exists so that version two is a migration
     * rather than a new reader.
     *
     * @return The chain.
     */
    [[nodiscard]] antwika::replay::MigrationChain
    standardStateDumpMigrations();

    /**
     * @brief Encode a dump's state object.
     * @param dump The state to encode.
     * @return The JSON object the envelope's state member holds.
     */
    [[nodiscard]] nlohmann::json stateDumpToJson(
        const EditorStateDump &dump);

    /**
     * @brief Decode a dump's state object, validating it first.
     * @param state The envelope's state member.
     * @return The decoded dump.
     * @throws AtlasEditorError If the object fails the schema, names a
     * tool this build does not know, or names a zoom level the table
     * has no entry for.
     */
    [[nodiscard]] EditorStateDump stateDumpFromJson(
        const nlohmann::json &state);

} // namespace antwika::atlas_editor
