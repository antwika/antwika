#pragma once

#include <optional>

#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/CanvasView.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/Pixel.hpp"
#include "antwika/atlas_editor/Selection.hpp"
#include "antwika/atlas_editor/SpriteGuides.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Size;

    /**
     * @brief Everything drawing the sheet needs, and nothing it could
     * change.
     *
     * Rendering is a write-only projection here in structure rather than
     * by promise: the scene is handed this value, so there is no session
     * state it could reach even by mistake, and a test can assert a
     * whole frame by writing one of these out by hand.
     *
     * The pixels themselves are deliberately absent. They are uploaded
     * once per change as a texture and blitted, since a sheet is a
     * quarter of a million pixels and a rectangle each would be a
     * quarter of a million draw calls a frame.
     */
    struct SceneSnapshot
    {
        /** @brief How big the sheet is. */
        Size image{};

        /** @brief Where it sits on the canvas, and how big it is drawn. */
        CanvasView view{};

        /** @brief How the sheet is divided into slots. */
        TileGrid tiles{};

        /** @brief Whether to draw the slot boundaries. */
        bool gridVisible = false;

        /**
         * @brief Where each slot's footprint diamond is, when they are
         * being drawn at all.
         *
         * Nothing both when the artist has turned them off and when the
         * slot size has none, so the scene has one thing to ask rather
         * than two: whether to draw them is settled before it is handed
         * this.
         */
        std::optional<SpriteGuides> guides{};

        /**
         * @brief Which rectangle to outline, if one is marked out.
         *
         * What a drag in progress is heading for while there is one, so
         * the outline follows the pointer and the sheet under it does
         * not change until the button comes up.
         */
        std::optional<Selection> selection{};

        /** @brief Which pixel to outline, if the pointer is on one. */
        std::optional<Pixel> hovered{};

        /**
         * @brief Compare two snapshots.
         * @param other The snapshot to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const SceneSnapshot &other) const =
            default;
    };

    /**
     * @brief Take the drawing half of a session's state.
     * @param state The session to read.
     * @return What the scene needs to draw it.
     */
    [[nodiscard]] SceneSnapshot snapshotOf(const EditorState &state);

} // namespace antwika::atlas_editor
