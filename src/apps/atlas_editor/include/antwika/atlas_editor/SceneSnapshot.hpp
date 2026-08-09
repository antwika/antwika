#pragma once

#include <optional>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/CanvasView.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/Pixel.hpp"
#include "antwika/atlas_editor/Preview.hpp"
#include "antwika/atlas_editor/Selection.hpp"
#include "antwika/atlas_editor/SpriteGuides.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"
#include "antwika/atlas_editor/Tool.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Point;
    using antwika::gfx::Size;

    struct PreviewShot final
    {
        Rect pane{};

        CanvasView view{};

        std::optional<Rect> slot{};

        [[nodiscard]] bool operator==(const PreviewShot &other) const =
            default;
    };

    struct SceneSnapshot final
    {
        Size image{};

        CanvasView view{};

        TileGrid tiles{};

        bool gridVisible = false;

        bool pixelGridVisible = false;

        std::optional<SpriteGuides> guides{};

        std::optional<Point> pivot{};

        bool pointerBorder = false;

        std::optional<Selection> selection{};

        Tool tool = Tool::Paint;

        std::optional<Gesture> stroke{};

        std::optional<Pixel> hovered{};

        std::optional<PreviewShot> preview{};

        [[nodiscard]] bool operator==(const SceneSnapshot &other) const =
            default;
    };

    [[nodiscard]] SceneSnapshot snapshotOf(const EditorState &state);

    /**
     * @brief Takes a snapshot with the preview pane filled in.
     *
     * @param state The editor to read.
     * @param pane Where the preview pane landed, if it is open.
     * @return The snapshot the scene draws from.
     */
    [[nodiscard]] SceneSnapshot snapshotOf(
        const EditorState &state, const std::optional<Rect> &pane);

}
