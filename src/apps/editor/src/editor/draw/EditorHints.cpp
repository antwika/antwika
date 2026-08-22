#include <algorithm>

#include <antwika/decor/Decor.hpp>
#include <antwika/editor/ui/AtlasView.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/map/Layers.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/tile/TilePaint.hpp>
#include <antwika/ui/HoverHint.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>
#include <antwika/gfx/TextLayout.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    namespace
    {

        template <typename Enum>
        struct HintRow final
        {
            Enum value;
            std::string_view hint;
        };

        constexpr std::array<HintRow<ToolButton>, enums::kCount<ToolButton>>
            kToolHints{{
            {ToolButton::Brush, "brush - lays cubes, right press takes them"},
            {ToolButton::Picker,
             "picker - lifts what stands under the pointer"},
            {ToolButton::FreeLook, "free look - drag turns the view about"},
            {ToolButton::Lighting, "lighting on and off"},
            {ToolButton::Lamp, "lamp - sets a light of the chosen ink"},
            {ToolButton::RuleLines, "rule lines on and off"},
            {ToolButton::Start, "start cube - the character begins on it"},
            {ToolButton::Exit, "exit cube - reaching it closes the game"},
            {ToolButton::Stamp, "stamp - drag copies cubes to set down again"},
            {ToolButton::Figure, "figure - sets a walker of the world down"},
            {ToolButton::PressurePlate, "plate - stood on, it sways cubes"},
            {ToolButton::Key, "key - a cube the player picks a key from"},
            {ToolButton::Door, "door - a cube a carried key dissolves"},
            {ToolButton::Checkpoint,
             "checkpoint - stood on, it sets the respawn"},
            {ToolButton::Food, "food - a cube a character picks food from"},
            {ToolButton::Water, "water - a cube a character picks water from"},
            {ToolButton::Eraser,
             "rubber - clears cubes, drag sweeps them away"}}};

        static_assert(
            enums::tagsInOrder(kToolHints, &HintRow<ToolButton>::value));

        [[nodiscard]] std::string_view toolHint(const ToolButton whichButton)
        {
            return enums::lookup(kToolHints, whichButton).hint;
        }

        constexpr std::array<HintRow<map::Paint>, enums::kCount<map::Paint>>
            kPaintHints{{
            {map::Paint::Brush, "brush - draws pixel by pixel"},
            {map::Paint::Line, "line - drags a straight run"},
            {map::Paint::Fill, "fill - floods a patch of one colour"},
            {map::Paint::Select, "mark - drags a rectangle to lift"},
            {map::Paint::Rect, "rectangle - drags an outline"},
            {map::Paint::Circle, "circle - drags a ring"}}};

        static_assert(
            enums::tagsInOrder(kPaintHints, &HintRow<map::Paint>::value));

        [[nodiscard]] std::string_view paintHint(const map::Paint whichPaint)
        {
            return enums::lookup(kPaintHints, whichPaint).hint;
        }

        constexpr std::array<HintRow<voxel::Kind>, enums::kCount<voxel::Kind>>
            kKindHints{{
            {voxel::Kind::Normal, "stone - stood on and built with"},
            {voxel::Kind::Water,
             "water - waded through, never stood on"},
            {voxel::Kind::Ramp,
             "ramp - a flight climbed at half pace"},
            {voxel::Kind::Ladder,
             "ladder - climbed straight up and down"}}};

        static_assert(
            enums::tagsInOrder(kKindHints, &HintRow<voxel::Kind>::value));

        [[nodiscard]] std::string_view kindHint(const voxel::Kind whichKind)
        {
            return enums::lookup(kKindHints, whichKind).hint;
        }

        constexpr std::array<HintRow<voxel::Facing>,
            enums::kCount<voxel::Facing>>
            kFacingHints{{
            {voxel::Facing::Any,
             "climbs whichever way the ground asks"},
            {voxel::Facing::East, "climbs east"},
            {voxel::Facing::West, "climbs west"},
            {voxel::Facing::North, "climbs north"},
            {voxel::Facing::South, "climbs south"}}};

        static_assert(
            enums::tagsInOrder(kFacingHints, &HintRow<voxel::Facing>::value));

        [[nodiscard]] std::string_view facingHint(
            const voxel::Facing whichFacing)
        {
            return enums::lookup(kFacingHints, whichFacing).hint;
        }

        constexpr std::array<HintRow<voxel::StairHalf>,
            enums::kCount<voxel::StairHalf>>
            kStairHalfHints{{
            {voxel::StairHalf::Any,
             "drawn for either step of a flight"},
            {voxel::StairHalf::Lower,
             "drawn for the lower step of a flight"},
            {voxel::StairHalf::Upper,
             "drawn for the upper step of a flight"}}};

        static_assert(
            enums::tagsInOrder(kStairHalfHints,
                &HintRow<voxel::StairHalf>::value));

        [[nodiscard]] std::string_view levelHint(
            const voxel::StairHalf whichHalf)
        {
            return enums::lookup(kStairHalfHints, whichHalf).hint;
        }

        constexpr std::array<HintRow<EdgeToggle>, enums::kCount<EdgeToggle>>
            kEdgeToggleHints{{
            {EdgeToggle::Boundary, "rim - this edge may lie against the air"},
            {EdgeToggle::Forbidden, "shut - this edge meets nothing at all"}}};

        static_assert(
            enums::tagsInOrder(kEdgeToggleHints, &HintRow<EdgeToggle>::value));

        [[nodiscard]] std::string_view edgeToggleHint(
            const EdgeToggle whichToggle)
        {
            return enums::lookup(kEdgeToggleHints, whichToggle).hint;
        }

    }

    namespace
    {

        [[nodiscard]] bool onToolPanel(const ui::WidgetId whichWidget)
        {
            for (const auto button : kEveryToolButton)
            {
                if (whichWidget == toolWidget(button))
                {
                    return true;
                }
            }

            for (const auto paint : kEveryPaint)
            {
                if (whichWidget == paintWidget(paint))
                {
                    return true;
                }
            }

            for (const auto kind : voxel::kEveryKind)
            {
                if (whichWidget == kindWidget(kind))
                {
                    return true;
                }
            }

            for (const auto facing : kMarkedFacings)
            {
                if (whichWidget == facingWidget(facing))
                {
                    return true;
                }
            }

            for (const auto level : kMarkedStairHalves)
            {
                if (whichWidget == levelWidget(level))
                {
                    return true;
                }
            }

            return whichWidget == kMirrorWidget;
        }

    }

}

namespace antwika::editor
{

    void Editor::drawToolHint(const ui::Frame &frame)
    {
        if (!tooltipDue(pointer.hoverTracker, tick)
            || pointer.hoverTracker.widget != pointer.hoveredWidget
            || !onToolPanel(pointer.hoveredWidget))
        {
            return;
        }

        const auto hint = hintFor(pointer.hoveredWidget);
        const auto room = frame.rects.find(pointer.hoveredWidget);

        if (hint.empty() || !room.has_value())
        {
            return;
        }

        const auto pad = static_cast<float>(2 * kUiScale);
        const auto hintSize = gfx::textSize(hint, kUiScale);
        const auto width =
            static_cast<float>(hintSize.width) + (pad * 2.0F);
        const auto height =
            static_cast<float>(hintSize.height) + (pad * 2.0F);
        const auto window = viewportRenderer.windowSize();

        const auto left = std::min(
            static_cast<float>(
                room->originPoint.x + room->size.width)
                + pad,
            static_cast<float>(window.width) - width);
        const auto top = std::min(
            static_cast<float>(room->originPoint.y),
            static_cast<float>(window.height) - height);

        auto &nativeRenderer = viewportRenderer.nativeRenderer();

        nativeRenderer.drawRect(
            gfx::RectF({left, top}, {width, height}), kTitleBarColor);
        nativeRenderer.drawText(
            {left + pad, top + pad}, hint, kUiScale, kTextColor);
    }

    namespace
    {

        [[nodiscard]] std::string tileLabel(
            const tilemap::Tile tile, const std::string_view tileName = "tile")
        {
            return std::string(
                       tile.atlas == tilemap::Atlas::Floor ? "Floor"
                                   : "Wall")
                   + " " + std::string(tileName) + " #"
                   + std::to_string(tile.index);
        }

    }

    void Editor::updateCanvasHover(const ui::Frame &frame)
    {
        auto tileCell = std::optional<geometry::GridCell>{};
        auto face = std::optional<std::size_t>{};

        const auto clear =
            !playing && !dialogs.quitConfirmOpen && !keysOpen
            && !dialogs.fileDialog.has_value()
            && !inkPicker.editingInk.has_value()
            && !frame.interactions.pointerOverUi;

        if (clear && activeView == map::View::Atlases)
        {
            const auto cell = cellUnderPointer();

            if (cell.has_value()
                && map.tilemap.at(cell->column, cell->row)
                       .has_value())
            {
                tileCell = cell;
            }
        }

        if (clear && activeView == map::View::World
            && tool == map::Tool::Picker)
        {
            face = voxelmap::facePicked(
                visibleCells(),
                worldMeshes.faces(),
                worldCamera(),
                worldRotation(),
                camera::kCanvasSize,
                pointer.pointerOnCanvas);
        }

        if (tileCell != pointer.canvasRest.tileCell
            || face != pointer.canvasRest.face)
        {
            pointer.canvasRest = CanvasRest{
                .tileCell = tileCell,
                .face = face,
                .sinceTick = tick};
        }
    }

    void Editor::drawCanvasHint()
    {
        if (tick - pointer.canvasRest.sinceTick
                < ui::kTooltipDelayFrames
            || !pointer.pointerInWindow.has_value())
        {
            return;
        }

        auto hint = std::string();

        if (pointer.canvasRest.tileCell.has_value())
        {
            const auto tile = map.tilemap.at(
                pointer.canvasRest.tileCell->column,
                pointer.canvasRest.tileCell->row);

            if (!tile.has_value())
            {
                return;
            }

            hint = tileLabel(*tile);
        }
        else if (pointer.canvasRest.face.has_value()
                 && *pointer.canvasRest.face < worldMeshes.faces().size())
        {
            const auto face = *pointer.canvasRest.face;
            const auto tile =
                face < worldMeshes.drawnAs().size()
                    ? worldMeshes.drawnAs().at(face)
                    : voxelmap::faceTile(worldMeshes.faces().at(face));

            hint = "Voxel face #" + std::to_string(face)
                   + ", " + tileLabel(tile);

            for (const auto &[layer, layerDecor] : worldMeshes.decorLayers())
            {
                const auto foundPlacement = layerDecor.find(face);

                if (foundPlacement != layerDecor.end())
                {
                    hint += ", "
                            + tileLabel(foundPlacement->second, "decor")
                            + " (" + map::layerLabel(layer) + ")";
                }
            }
        }
        else
        {
            return;
        }

        const auto pad = static_cast<float>(2 * kUiScale);
        const auto hintSize = gfx::textSize(hint, kUiScale);
        const auto width =
            static_cast<float>(hintSize.width) + (pad * 2.0F);
        const auto height =
            static_cast<float>(hintSize.height) + (pad * 2.0F);
        const auto window = viewportRenderer.windowSize();

        const auto left = std::clamp(
            static_cast<float>(pointer.pointerInWindow->x) + (pad * 2.0F),
            0.0F,
            static_cast<float>(window.width) - width);
        const auto top = std::clamp(
            static_cast<float>(pointer.pointerInWindow->y) + (pad * 2.0F),
            0.0F,
            static_cast<float>(window.height) - height);

        auto &nativeRenderer = viewportRenderer.nativeRenderer();

        nativeRenderer.drawRect(
            gfx::RectF({left, top}, {width, height}), kTitleBarColor);
        nativeRenderer.drawText(
            {left + pad, top + pad}, hint, kUiScale, kTextColor);
    }

    std::string_view Editor::hintFor(const ui::WidgetId whichWidget) const
    {
        if (whichWidget == tabWidget(map::View::World))
        {
            return "world - the pile itself, built and played";
        }

        if (whichWidget == tabWidget(map::View::Atlases))
        {
            return "tiles - the atlases the pile is drawn from";
        }

        if (whichWidget == tabWidget(map::View::Character))
        {
            return "characters - the walkers and their sheets";
        }

        if (whichWidget == tabWidget(map::View::Icons))
        {
            return "icons - the editor's own pictures";
        }

        for (const auto button : kEveryToolButton)
        {
            if (whichWidget == toolWidget(button))
            {
                return toolHint(button);
            }
        }

        for (const auto paint : kEveryPaint)
        {
            if (whichWidget == paintWidget(paint))
            {
                return paintHint(paint);
            }
        }

        for (const auto kind : voxel::kEveryKind)
        {
            if (whichWidget == kindWidget(kind))
            {
                return kindHint(kind);
            }
        }

        for (const auto facing : kMarkedFacings)
        {
            if (whichWidget == facingWidget(facing))
            {
                return facingHint(facing);
            }
        }

        for (const auto level : kMarkedStairHalves)
        {
            if (whichWidget == levelWidget(level))
            {
                return levelHint(level);
            }
        }

        for (const auto toggle : kEveryEdgeToggle)
        {
            if (whichWidget == edgeToggleWidget(toggle))
            {
                return edgeToggleHint(toggle);
            }
        }

        if (whichWidget == kPartFrontWidget)
        {
            return "drawn for the fronts of a flight - its "
                   "risers and the face at its head";
        }

        if (whichWidget == kPartSideWidget)
        {
            return "drawn for the stepped side of a flight";
        }

        for (std::size_t frame = 0; frame < decor::kMaxDecorFrames;
             ++frame)
        {
            if (whichWidget == decor::frameWidget(frame))
            {
                return "picks the frame the canvas draws";
            }
        }

        for (std::size_t ink = 0; ink < map.paletteColors.size();
             ++ink)
        {
            if (whichWidget == tile::swatchWidget(ink))
            {
                return "chooses this ink - again mixes it";
            }
        }

        for (std::size_t layer = 0; layer < map.layers.size();
             ++layer)
        {
            if (whichWidget == map::layerWidget(layer))
            {
                return "works on this layer";
            }
        }

        if (whichWidget == kDeriveRulesWidget)
        {
            return "ties tiles of one shape together";
        }

        if (whichWidget == kMirrorWidget)
        {
            return "flips the marked patch, left for right";
        }

        if (whichWidget == kAddInkWidget)
        {
            return "adds another ink to the palette";
        }

        if (whichWidget == map::kAddLayerWidget)
        {
            return "adds a layer over the ones held";
        }

        if (whichWidget == map::kRemoveLayerWidget)
        {
            return "takes the chosen layer away";
        }

        if (whichWidget == decor::kPickBaseTilesWidget)
        {
            return "picks the bases the decor stands on - "
                   "an upright base dresses walls";
        }

        if (whichWidget == decor::kVariantChoiceWidget)
        {
            return "picks the tiles drawn in this one's stead";
        }

        if (whichWidget == decor::kVariantWeightWidget)
        {
            return "how often this variant is drawn - "
                   "the wheel nudges it";
        }

        if (whichWidget == decor::kGoToCanonicalWidget)
        {
            return "goes to the leader this tile stands in for";
        }

        if (whichWidget == decor::kFrequencyWidget)
        {
            return "how often the decor takes a base - "
                   "the wheel nudges it";
        }

        if (whichWidget == decor::kDecorWeightWidget)
        {
            return "how strongly this decor is weighed against "
                   "the others its base offers - the wheel "
                   "nudges it";
        }

        if (whichWidget == decor::kDecorMoveWidget)
        {
            return "moves this decor to the layer being "
                   "worked on";
        }

        if (whichWidget == decor::kFrameAddWidget)
        {
            return "adds a frame, drawn in this very place";
        }

        if (whichWidget == decor::kToggleAnimationWidget)
        {
            return "walks this tile's pixels through frames "
                   "wherever it is drawn";
        }

        if (whichWidget == decor::kAddFrameWidget)
        {
            return "adds a frame for the tile to walk";
        }

        for (std::size_t frame = 0;
             frame < decor::kMaxDecorFrames;
             ++frame)
        {
            if (whichWidget
                == decor::flipFrameWidget(frame))
            {
                return "shows this frame - a grid click "
                       "assigns its tile";
            }
        }

        if (whichWidget == kFigureLampWidget)
        {
            return "gives this figure a lamp to carry";
        }

        if (whichWidget == kExitLockedWidget)
        {
            return "the exit opens only for a carried key";
        }

        if (whichWidget == tile::kTransitionAddWidget)
        {
            return "weaves an edge between two materials "
                   "from a drawn mask";
        }

        if (whichWidget == tile::kRemoveTransitionWidget)
        {
            return "takes the chosen transition away";
        }

        for (std::size_t index = 0;
             index < tile::kMaxTransitions;
             ++index)
        {
            if (whichWidget
                == tile::transitionRowWidget(index))
            {
                return "shows this transition's pieces";
            }
        }

        if (whichWidget == decor::kSpanAcrossLessWidget
            || whichWidget == decor::kSpanAcrossMoreWidget
            || whichWidget == decor::kSpanDownLessWidget
            || whichWidget == decor::kSpanDownMoreWidget)
        {
            return "how many faces the decor spans - "
                   "a span is stamped whole or not at all";
        }

        for (std::size_t place = 1;
             place < static_cast<std::size_t>(
                         decor::kMaxDecorSpan)
                         * decor::kMaxDecorSpan;
             ++place)
        {
            if (whichWidget == decor::memberWidget(place))
            {
                return "shows this place of the span - "
                       "a grid click assigns its tile";
            }
        }

        if (whichWidget == decor::kAutoPreviewWidget)
        {
            return "keeps the tiling squares turning";
        }

        if (whichWidget == decor::kRerollPreviewWidget)
        {
            return "lays another tiling square";
        }

        return "";
    }

}
