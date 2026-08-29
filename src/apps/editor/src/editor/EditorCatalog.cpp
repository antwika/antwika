#include <algorithm>
#include <array>
#include <filesystem>
#include <iterator>
#include <string>

#include <antwika/decor/Decor.hpp>
#include <antwika/decor/TileAnimation.hpp>
#include <antwika/enums/Enumeration.hpp>
#include <antwika/map/Layers.hpp>
#include <antwika/tile/TilePaint.hpp>
#include <antwika/tile/Transitions.hpp>
#include <antwika/voxel/VoxelCube.hpp>

#include "antwika/editor/Editor.hpp"
#include "antwika/editor/editor/CarriedLight.hpp"
#include "antwika/editor/ui/LayerWidgets.hpp"
#include "antwika/editor/ui/WidgetCatalog.hpp"
#include "antwika/editor/ui/WidgetIds.hpp"

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

        constexpr std::array<HintRow<View>, enums::kCount<View>>
            kViewHints{{
            {View::World,
             "world - the pile itself, built and played"},
            {View::Atlases,
             "tiles - the atlases the pile is drawn from"},
            {View::Character,
             "characters - the walkers and their sheets"},
            {View::Icons, "icons - the editor's own pictures"},
            {View::Plan, ""},
            {View::Gizmos,
             "gizmos - the marks the world's entities wear"}}};

        static_assert(
            enums::tagsInOrder(kViewHints, &HintRow<View>::value));

        [[nodiscard]] std::string_view getTabHint(const View whichView)
        {
            return enums::lookup(kViewHints, whichView).hint;
        }

        constexpr std::array<HintRow<ToolButton>, enums::kCount<ToolButton>>
            kToolHints{{
            {ToolButton::StoneCube,
             "stone - lays cubes stood on and built with"},
            {ToolButton::WaterCube,
             "water - lays cubes waded through, never stood on"},
            {ToolButton::RampCube,
             "ramp - lays a flight climbed at half pace"},
            {ToolButton::Picker,
             "picker - lifts what stands under the pointer"},
            {ToolButton::Stamp, "stamp - drag copies cubes to set down again"},
            {ToolButton::Rubber,
             "rubber - clears cubes, drag sweeps them away"},
            {ToolButton::Select,
             "select - picks an entity to move, edit or take away"},
            {ToolButton::Lamp, "lamp - sets a light of the chosen ink"},
            {ToolButton::Start, "start cube - the character begins on it"},
            {ToolButton::Exit, "exit cube - reaching it closes the game"},
            {ToolButton::Character,
             "character - sets a walker of the world down"},
            {ToolButton::Checkpoint,
             "checkpoint - stood on, it sets the respawn"},
            {ToolButton::Food, "food - a cube a character picks food from"},
            {ToolButton::Water,
             "water - a cube a character picks water from"}}};

        static_assert(
            enums::tagsInOrder(kToolHints, &HintRow<ToolButton>::value));

        [[nodiscard]] std::string_view getToolHint(const ToolButton whichButton)
        {
            return enums::lookup(kToolHints, whichButton).hint;
        }

        constexpr std::array<HintRow<Paint>, enums::kCount<Paint>>
            kPaintHints{{
            {Paint::Brush, "brush - draws pixel by pixel"},
            {Paint::Line, "line - drags a straight run"},
            {Paint::Fill, "fill - floods a patch of one colour"},
            {Paint::Select, "mark - drags a rectangle to lift"},
            {Paint::Rect, "rectangle - drags an outline"},
            {Paint::Circle, "circle - drags a ring"}}};

        static_assert(
            enums::tagsInOrder(kPaintHints, &HintRow<Paint>::value));

        [[nodiscard]] std::string_view getPaintHint(const Paint whichPaint)
        {
            return enums::lookup(kPaintHints, whichPaint).hint;
        }

        constexpr std::array<HintRow<voxel::Kind>, enums::kCount<voxel::Kind>>
            kKindHints{{
            {voxel::Kind::Normal, "stone - stood on and built with"},
            {voxel::Kind::Water,
             "water - waded through, never stood on"},
            {voxel::Kind::Ramp,
             "ramp - a flight climbed at half pace"}}};

        static_assert(
            enums::tagsInOrder(kKindHints, &HintRow<voxel::Kind>::value));

        [[nodiscard]] std::string_view getKindHint(const voxel::Kind whichKind)
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

        [[nodiscard]] std::string_view getFacingHint(
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

        [[nodiscard]] std::string_view getLevelHint(
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

        [[nodiscard]] std::string_view getEdgeToggleHint(
            const EdgeToggle whichToggle)
        {
            return enums::lookup(kEdgeToggleHints, whichToggle).hint;
        }

        constexpr std::array kPartHints{
            std::string_view{
                "drawn for the fronts of a flight - its "
                "risers and the face at its head"},
            std::string_view{
                "drawn for the stepped side of a flight"}};

        static_assert(kPartHints.size() == kMarkedParts.size());

        template <typename Row, std::size_t Count>
        [[nodiscard]] constexpr std::array<widget::WidgetId, Count>
        getRowWidgets(const std::array<Row, Count> &rows) noexcept
        {
            auto widgets = std::array<widget::WidgetId, Count>{};

            for (std::size_t place = 0; place < Count; ++place)
            {
                widgets[place] = rows[place].widget;
            }

            return widgets;
        }

        template <typename Row, std::size_t Count>
        [[nodiscard]] constexpr bool isEveryRowClaimed(
            const std::array<Row, Count> &rows) noexcept
        {
            for (const auto &row : rows)
            {
                if (row.activation == nullptr
                    && row.delegate
                           == widget_catalog::Catalog::Delegate::Activation)
                {
                    return false;
                }
            }

            return true;
        }

        template <typename SoloRow, std::size_t SoloCount,
            typename SliderRow, std::size_t SliderCount>
        [[nodiscard]] constexpr bool isEverySliderTagBacked(
            const std::array<SoloRow, SoloCount> &soloRows,
            const std::array<SliderRow, SliderCount> &sliderRows) noexcept
        {
            for (const auto &row : soloRows)
            {
                if (row.delegate
                    != widget_catalog::Catalog::Delegate::Slider)
                {
                    continue;
                }

                auto sliderRowStands = false;

                for (const auto &sliderRow : sliderRows)
                {
                    sliderRowStands = sliderRowStands || sliderRow.widget == row.widget;
                }

                if (!sliderRowStands)
                {
                    return false;
                }
            }

            return true;
        }

    }

    const widget_catalog::Catalog &Editor::getWidgetCatalog()
    {
        using widget_catalog::Catalog;
        using Delegate = Catalog::Delegate;
        using FamilyRow = Catalog::FamilyRow;
        using FieldRow = Catalog::FieldRow;
        using SliderRow = Catalog::SliderRow;
        using SoloRow = Catalog::SoloRow;

        static constexpr std::array kSoloRows{
            SoloRow{
                .widget = kDeriveRulesWidget,
                .hint = "ties tiles of one shape together",
                .activation = [](Editor &editor)
                {
                    editor.deriveRulesFromShapes();

                    return true;
                }},
            SoloRow{
                .widget = kMirrorWidget,
                .hint = "flips the marked patch, left for right",
                .toolPanelMembership = true,
                .activation = [](Editor &editor)
                {
                    editor.characterView.mirrorSelection(editor);
                    editor.characterView.commitFloatingPatch();
                    editor.characterView.dropSelection();

                    return true;
                }},
            SoloRow{
                .widget = kAddInkWidget,
                .hint = "adds another ink to the palette",
                .delegate = Delegate::InkPanel},
            SoloRow{
                .widget = kAddLayerWidget,
                .hint = "adds a layer over the ones held",
                .activation = [](Editor &editor)
                {
                    editor.pushUndo();
                    editor.document.map.layers = map::getWithLayerAdded(
                        editor.document.map.layers);
                    editor.chosenLayer =
                        editor.document.map.layers.size() - 1;

                    return true;
                }},
            SoloRow{
                .widget = kRemoveLayerWidget,
                .hint = "takes the chosen layer away",
                .activation = [](Editor &editor)
                {
                    editor.pushUndo();
                    editor.document.map.layers = map::getWithLayerRemoved(
                        editor.document.map.layers, editor.chosenLayer);
                    editor.chosenLayer = std::min(
                        editor.chosenLayer,
                        editor.document.map.layers.size() - 1);

                    return true;
                }},
            SoloRow{
                .widget = kPickBaseTilesWidget,
                .hint = "picks the bases the decor stands on - "
                        "an upright base dresses walls",
                .activation = [](Editor &editor)
                {
                    const auto was = editor.assignMode.basePicking;

                    editor.clearAssignModes();
                    editor.assignMode.basePicking = !was;

                    return true;
                }},
            SoloRow{
                .widget = kVariantChoiceWidget,
                .hint = "picks the tiles drawn in this one's stead",
                .activation = [](Editor &editor)
                {
                    const auto was = editor.assignMode.variantPicking;

                    editor.clearAssignModes();
                    editor.assignMode.variantPicking = !was;

                    return true;
                }},
            SoloRow{
                .widget = kVariantWeightWidget,
                .hint = "how often this variant is drawn - "
                        "the wheel nudges it",
                .delegate = Delegate::Slider},
            SoloRow{
                .widget = kGoToCanonicalWidget,
                .hint = "goes to the leader this tile stands in for",
                .activation = [](Editor &editor)
                {
                    if (!editor.stroke.selectedTile.has_value())
                    {
                        return false;
                    }

                    editor.stroke.selectedTile = canonicalTileOf(
                        editor.document.map.familyGroups,
                        *editor.stroke.selectedTile);
                    editor.assignMode.variantPicking = false;

                    return true;
                }},
            SoloRow{
                .widget = kFrequencyWidget,
                .hint = "how often the decor takes a base - "
                        "the wheel nudges it",
                .delegate = Delegate::Slider},
            SoloRow{
                .widget = kDecorWeightWidget,
                .hint = "how strongly this decor is weighed against "
                        "the others its base offers - the wheel "
                        "nudges it",
                .delegate = Delegate::Slider},
            SoloRow{
                .widget = kGlowWidget,
                .hint = "how strongly the ink being mixed glows "
                        "in the dark",
                .delegate = Delegate::Slider},
            SoloRow{
                .widget = kAmbientWidget,
                .hint = "how much light fills the world before "
                        "any lamp is lit",
                .delegate = Delegate::Slider},
            SoloRow{
                .widget = kDecorMoveWidget,
                .hint = "moves this decor to the layer being "
                        "worked on",
                .activation = [](Editor &editor)
                {
                    if (!isDecorLayer(editor.chosenLayer)
                        || !editor.stroke.selectedTile.has_value())
                    {
                        return false;
                    }

                    editor.pushUndo();
                    editor.document.map.decor = decor::getWithDecorLayerSet(
                        editor.document.map.decor,
                        *editor.stroke.selectedTile,
                        editor.chosenLayer);
                    editor.rebuildWorld();

                    return true;
                }},
            SoloRow{
                .widget = kFrameAddWidget,
                .hint = "adds a frame, drawn in this very place",
                .activation = [](Editor &editor)
                {
                    if (!editor.stroke.selectedTile.has_value())
                    {
                        return false;
                    }

                    const auto spare = editor.freeTileSlot(
                        editor.stroke.selectedTile->atlas);

                    if (!spare.has_value())
                    {
                        editor.showStatus(
                            "no spare tile is left to hold "
                            "another frame",
                            true);

                        return true;
                    }

                    editor.pushUndo();
                    editor.ensureDecor();
                    editor.document.map.decor = decor::getWithFrameAdded(
                        editor.document.map.decor,
                        *editor.stroke.selectedTile);

                    const auto *decor =
                        decor::decorOf(
                            editor.document.map.decor,
                            *editor.stroke.selectedTile);
                    const auto lastIndex =
                        decor->frameTiles.size() - 1;

                    editor.copyTilePixels(
                        decor->frameTiles.at(lastIndex - 1), *spare);
                    editor.document.map.decor = decor::getWithFrameSet(
                        editor.document.map.decor,
                        *editor.stroke.selectedTile,
                        lastIndex,
                        *spare);
                    editor.clearAssignModes();
                    editor.assignMode.framePicked = lastIndex;
                    editor.atlasSheets.touch();

                    return true;
                }},
            SoloRow{
                .widget = kToggleAnimationWidget,
                .hint = "walks this tile's pixels through frames "
                        "wherever it is drawn",
                .activation = [](Editor &editor)
                {
                    if (!editor.stroke.selectedTile.has_value())
                    {
                        return false;
                    }

                    editor.pushUndo();
                    editor.document.map.flipAnimations =
                        getWithAnimationToggled(
                            editor.document.map.flipAnimations,
                            *editor.stroke.selectedTile);
                    editor.assignMode.flipFramePicked = 0;
                    editor.assignMode.flipFrameAssigning = false;
                    editor.atlasSheets.touch();

                    return true;
                }},
            SoloRow{
                .widget = kAddFrameWidget,
                .hint = "adds a frame for the tile to walk",
                .activation = [](Editor &editor)
                {
                    if (!editor.stroke.selectedTile.has_value())
                    {
                        return false;
                    }

                    editor.pushUndo();
                    editor.document.map.flipAnimations =
                        getWithAnimationFrameAdded(
                            editor.document.map.flipAnimations,
                            *editor.stroke.selectedTile);

                    const auto *animation = animationOf(
                        editor.document.map.flipAnimations,
                        *editor.stroke.selectedTile);

                    if (animation != nullptr
                        && !animation->frameTiles.empty())
                    {
                        editor.assignMode.flipFramePicked =
                            animation->frameTiles.size() - 1;
                        editor.assignMode.flipFrameAssigning =
                            editor.assignMode.flipFramePicked > 0;
                    }

                    editor.atlasSheets.touch();

                    return true;
                }},
            SoloRow{
                .widget = kCharacterLampWidget,
                .hint = "gives this character a lamp to carry",
                .activation = [](Editor &editor)
                {
                    const auto chosenCharacter =
                        editor.worldView.characterTool().getChosenCharacter(
                            editor.document.map.characters.size());

                    if (!chosenCharacter.has_value())
                    {
                        return false;
                    }

                    editor.pushUndo();
                    toggleCarriedLight(
                        editor.document.map.characters.at(*chosenCharacter));

                    return true;
                }},
            SoloRow{
                .widget = kExitTargetWidget,
                .activation = [](Editor &editor)
                {
                    editor.pushUndo();
                    editor.focusedField = FocusedField::ExitTarget;

                    return true;
                }},
            SoloRow{
                .widget = kMarkerRemoveWidget,
                .hint = "takes the chosen marker away",
                .activation = [](Editor &editor)
                {
                    if (!editor.isMarkerSectionShown())
                    {
                        return false;
                    }

                    auto &cells = editor.document.map.markers.positionsOf(
                        *editor.markerPick.marker);
                    const auto foundCell = std::ranges::find(
                        cells, editor.markerPick.position);

                    if (foundCell == cells.end())
                    {
                        return false;
                    }

                    editor.pushUndo();
                    cells.erase(foundCell);
                    editor.dropMarkerPick();

                    return true;
                }},
            SoloRow{
                .widget = kEntityRemoveWidget,
                .hint = "takes the chosen entity away",
                .activation = [](Editor &editor)
                {
                    if (!editor.isEntitySectionShown())
                    {
                        return false;
                    }

                    editor.removeEntityPick();

                    return true;
                }},
            SoloRow{
                .widget = kTransitionAddWidget,
                .hint = "weaves an edge between two materials "
                        "from a drawn mask",
                .activation = [](Editor &editor)
                {
                    if (!editor.stroke.selectedTile.has_value()
                        || editor.document.map.transitions.size()
                               >= tile::kMaxTransitions)
                    {
                        return false;
                    }

                    editor.transition.fromTile =
                        editor.stroke.selectedTile;
                    editor.transition.toTile.reset();
                    editor.showStatus(
                        "pick the other material", false, 360);

                    return true;
                }},
            SoloRow{
                .widget = kRemoveTransitionWidget,
                .hint = "takes the chosen transition away",
                .activation = [](Editor &editor)
                {
                    if (!editor.transition.chosenIndex.has_value()
                        || *editor.transition.chosenIndex
                               >= editor.document.map.transitions.size())
                    {
                        return false;
                    }

                    editor.pushUndo();
                    editor.document.map.transitions.erase(
                        std::next(
                            editor.document.map.transitions.begin(),
                            static_cast<std::ptrdiff_t>(
                                *editor.transition.chosenIndex)));
                    editor.transition.chosenIndex.reset();
                    editor.atlasSheets.touch();
                    editor.rebuildWorld();

                    return true;
                }},
            SoloRow{
                .widget = kSpanAcrossLessWidget,
                .hint = "how many faces the decor spans - "
                        "a span is stamped whole or not at all",
                .activation = [](Editor &editor)
                { return editor.nudgeSpan(-1, 0); }},
            SoloRow{
                .widget = kSpanAcrossMoreWidget,
                .hint = "how many faces the decor spans - "
                        "a span is stamped whole or not at all",
                .activation = [](Editor &editor)
                { return editor.nudgeSpan(1, 0); }},
            SoloRow{
                .widget = kSpanDownLessWidget,
                .hint = "how many faces the decor spans - "
                        "a span is stamped whole or not at all",
                .activation = [](Editor &editor)
                { return editor.nudgeSpan(0, -1); }},
            SoloRow{
                .widget = kSpanDownMoreWidget,
                .hint = "how many faces the decor spans - "
                        "a span is stamped whole or not at all",
                .activation = [](Editor &editor)
                { return editor.nudgeSpan(0, 1); }},
            SoloRow{
                .widget = kAutoPreviewWidget,
                .hint = "keeps the tiling squares turning",
                .activation = [](Editor &editor)
                {
                    editor.preview.automatic = !editor.preview.automatic;

                    return true;
                }},
            SoloRow{
                .widget = kRerollPreviewWidget,
                .hint = "lays another tiling square",
                .activation = [](Editor &editor)
                {
                    editor.preview.automatic = false;
                    editor.preview.seed += 1;
                    editor.preview.forTile.reset();

                    return true;
                }},
            SoloRow{
                .widget = kPickerParentFolderWidget,
                .activation = [](Editor &editor)
                {
                    if (!editor.fileChooser.fileDialog.has_value())
                    {
                        return false;
                    }

                    editor.fileChooser.fileDialog->folder =
                        std::filesystem::path(
                            editor.fileChooser.fileDialog->folder)
                            .parent_path()
                            .string();
                    editor.fileChooser.listFolder(
                        editor.fileChooser.fileDialog->folder);

                    return true;
                }},
            SoloRow{
                .widget = kPickerConfirmWidget,
                .activation = [](Editor &editor)
                {
                    if (!editor.fileChooser.fileDialog.has_value())
                    {
                        return false;
                    }

                    editor.confirmFileDialog();

                    return true;
                }},
            SoloRow{
                .widget = kPickerCancelWidget,
                .activation = [](Editor &editor)
                {
                    if (!editor.fileChooser.fileDialog.has_value())
                    {
                        return false;
                    }

                    editor.fileChooser.cancel();

                    return true;
                }}};

        static_assert(
            widget::allDistinct(getRowWidgets(kSoloRows)),
            "two solo rows share a widget");

        static_assert(
            isEveryRowClaimed(kSoloRows),
            "a solo row neither activates nor names its delegate");

        static constexpr std::array kFamilyRows{
            FamilyRow{
                .widgetAt = [](const std::size_t place)
                { return getMenuWidget(kEveryMenu.at(place)); },
                .placeCount = kEveryMenu.size(),
                .activation = [](Editor &editor, const std::size_t place)
                {
                    const auto menu = kEveryMenu.at(place);

                    editor.dialogs.openMenu =
                        editor.dialogs.openMenu == menu
                            ? std::nullopt
                            : std::optional{menu};

                    return true;
                }},
            FamilyRow{
                .widgetAt = [](const std::size_t place)
                { return getTabWidget(kEveryView.at(place)); },
                .placeCount = kEveryView.size(),
                .hintAt = [](const std::size_t place)
                { return getTabHint(kEveryView.at(place)); },
                .activation = [](Editor &editor, const std::size_t place)
                {
                    editor.setView(kEveryView.at(place));

                    return true;
                }},
            FamilyRow{
                .widgetAt = [](const std::size_t place)
                { return getToolWidget(kEveryToolButton.at(place)); },
                .placeCount = kEveryToolButton.size(),
                .hintAt = [](const std::size_t place)
                { return getToolHint(kEveryToolButton.at(place)); },
                .toolPanelMembership = true,
                .activation = [](Editor &editor, const std::size_t place)
                {
                    editor.pressTool(kEveryToolButton.at(place));

                    return true;
                }},
            FamilyRow{
                .widgetAt = [](const std::size_t place)
                { return getPaintWidget(kEveryPaint.at(place)); },
                .placeCount = kEveryPaint.size(),
                .hintAt = [](const std::size_t place)
                { return getPaintHint(kEveryPaint.at(place)); },
                .toolPanelMembership = true,
                .activation = [](Editor &editor, const std::size_t place)
                {
                    editor.preferences.paint = kEveryPaint.at(place);

                    return true;
                }},
            FamilyRow{
                .widgetAt = [](const std::size_t place)
                { return getKindWidget(voxel::kEveryKind.at(place)); },
                .placeCount = voxel::kEveryKind.size(),
                .hintAt = [](const std::size_t place)
                { return getKindHint(voxel::kEveryKind.at(place)); },
                .toolPanelMembership = true,
                .activation = [](Editor &editor, const std::size_t place)
                {
                    const auto kind = voxel::kEveryKind.at(place);

                    if (editor.stroke.selectedTile.has_value()
                        && !editor.blockedAsVariant())
                    {
                        editor.pushUndo();
                        getActiveRules(
                            editor.document.map, editor.chosenLayer)
                            .setKind(*editor.stroke.selectedTile, kind);
                        editor.rebuildWorld();
                    }

                    return true;
                }},
            FamilyRow{
                .widgetAt = [](const std::size_t place)
                { return getFacingWidget(kMarkedFacings.at(place)); },
                .placeCount = kMarkedFacings.size(),
                .hintAt = [](const std::size_t place)
                { return getFacingHint(kMarkedFacings.at(place)); },
                .toolPanelMembership = true,
                .activation = [](Editor &editor, const std::size_t place)
                {
                    const auto facing = kMarkedFacings.at(place);

                    if (editor.stroke.selectedTile.has_value()
                        && !editor.blockedAsVariant())
                    {
                        editor.pushUndo();
                        getActiveRules(
                            editor.document.map, editor.chosenLayer)
                            .setFacing(
                                *editor.stroke.selectedTile,
                                getActiveRules(
                                    editor.document.map,
                                    editor.chosenLayer)
                                        .facingOf(
                                            *editor.stroke.selectedTile)
                                        == facing
                                    ? voxel::Facing::Any
                                    : facing);
                        editor.rebuildWorld();
                    }

                    return true;
                }},
            FamilyRow{
                .widgetAt = [](const std::size_t place)
                { return getLevelWidget(kMarkedStairHalves.at(place)); },
                .placeCount = kMarkedStairHalves.size(),
                .hintAt = [](const std::size_t place)
                { return getLevelHint(kMarkedStairHalves.at(place)); },
                .toolPanelMembership = true,
                .activation = [](Editor &editor, const std::size_t place)
                {
                    if (!editor.stroke.selectedTile.has_value())
                    {
                        return false;
                    }

                    if (editor.blockedAsVariant())
                    {
                        return true;
                    }

                    const auto level = kMarkedStairHalves.at(place);

                    editor.pushUndo();
                    getActiveRules(editor.document.map, editor.chosenLayer)
                        .setLevel(
                            *editor.stroke.selectedTile,
                            getActiveRules(
                                editor.document.map, editor.chosenLayer)
                                    .levelOf(*editor.stroke.selectedTile)
                                    == level
                                ? voxel::StairHalf::Any
                                : level);
                    editor.rebuildWorld();

                    return true;
                }},
            FamilyRow{
                .widgetAt = [](const std::size_t place)
                { return getPartWidget(kMarkedParts.at(place)); },
                .placeCount = kMarkedParts.size(),
                .hintAt = [](const std::size_t place)
                { return kPartHints.at(place); },
                .activation = [](Editor &editor, const std::size_t place)
                {
                    if (!editor.stroke.selectedTile.has_value())
                    {
                        return false;
                    }

                    if (editor.blockedAsVariant())
                    {
                        return true;
                    }

                    const auto part = kMarkedParts.at(place);

                    editor.pushUndo();
                    getActiveRules(editor.document.map, editor.chosenLayer)
                        .setPart(
                            *editor.stroke.selectedTile,
                            getActiveRules(
                                editor.document.map, editor.chosenLayer)
                                    .partOf(*editor.stroke.selectedTile)
                                    == part
                                ? voxel::StairPart::Any
                                : part);
                    editor.rebuildWorld();

                    return true;
                }},
            FamilyRow{
                .widgetAt = [](const std::size_t place)
                { return getEdgeToggleWidget(kEveryEdgeToggle.at(place)); },
                .placeCount = kEveryEdgeToggle.size(),
                .hintAt = [](const std::size_t place)
                { return getEdgeToggleHint(kEveryEdgeToggle.at(place)); },
                .activation = [](Editor &editor, const std::size_t place)
                {
                    editor.flipEdgeToggle(kEveryEdgeToggle.at(place));

                    return true;
                }},
            FamilyRow{
                .widgetAt = getFrameWidget,
                .placeCount = decor::kMaxDecorFrames,
                .hint = "picks the frame the canvas draws",
                .activation = [](Editor &editor, const std::size_t place)
                {
                    editor.clearAssignModes();
                    editor.assignMode.framePicked = place;

                    return true;
                }},
            FamilyRow{
                .widgetAt = getSwatchWidget,
                .placeCountOf = [](const Editor &editor)
                { return editor.document.map.paletteColors.size(); },
                .hint = "chooses this ink - again mixes it",
                .delegate = Delegate::InkPanel},
            FamilyRow{
                .widgetAt = getLayerWidget,
                .placeCountOf = [](const Editor &editor)
                { return editor.document.map.layers.size(); },
                .hint = "works on this layer",
                .activation = [](Editor &editor, const std::size_t place)
                {
                    editor.chosenLayer = place;
                    editor.stroke.selectedEdges.reset();
                    editor.clearAssignModes();

                    return true;
                }},
            FamilyRow{
                .widgetAt = getFlipFrameWidget,
                .placeCount = decor::kMaxDecorFrames,
                .hint = "shows this frame - a grid click "
                        "assigns its tile",
                .activation = [](Editor &editor, const std::size_t place)
                {
                    editor.clearAssignModes();
                    editor.assignMode.flipFramePicked = place;
                    editor.assignMode.flipFrameAssigning = place > 0;

                    return true;
                }},
            FamilyRow{
                .widgetAt = getTransitionRowWidget,
                .placeCount = tile::kMaxTransitions,
                .hint = "shows this transition's pieces",
                .activation = [](Editor &editor, const std::size_t place)
                {
                    if (place >= editor.document.map.transitions.size())
                    {
                        return false;
                    }

                    editor.transition.chosenIndex =
                        editor.transition.chosenIndex == place
                            ? std::optional<std::size_t>{}
                            : std::optional{place};

                    return true;
                }},
            FamilyRow{
                .widgetAt = getMemberWidget,
                .firstPlace = 1,
                .placeCount = (static_cast<std::size_t>(decor::kMaxDecorSpan)
                               * decor::kMaxDecorSpan)
                              - 1,
                .hint = "shows this place of the span - "
                        "a grid click assigns its tile",
                .activation = [](Editor &editor, const std::size_t place)
                {
                    if (!editor.stroke.selectedTile.has_value()
                        || !isDecorLayer(editor.chosenLayer))
                    {
                        return false;
                    }

                    const auto *decor = decor::decorOf(
                        editor.document.map.decor,
                        *editor.stroke.selectedTile);

                    if (decor == nullptr
                        || place >= decor->spanTiles.size())
                    {
                        return false;
                    }

                    editor.clearAssignModes();
                    editor.assignMode.memberPicked = place;
                    editor.assignMode.memberAssigning = true;

                    return true;
                }},
            FamilyRow{
                .widgetAt = getMarkerFieldWidget,
                .placeCount = kMarkerAxisCount,
                .hint = "one axis of the marker - enter moves it",
                .activation = [](Editor &editor, const std::size_t place)
                {
                    if (!editor.isMarkerSectionShown())
                    {
                        return false;
                    }

                    editor.focusedField = FocusedField::MarkerAxis;
                    editor.markerPick.editingAxis = place;
                    editor.markerPick.pendingAxisText = std::to_string(
                        getCubeAxisOf(editor.markerPick.position, place));

                    return true;
                }},
            FamilyRow{
                .widgetAt = getEntityFieldWidget,
                .placeCount = kMarkerAxisCount,
                .hint = "one axis of the entity - enter moves it",
                .activation = [](Editor &editor, const std::size_t place)
                {
                    if (!editor.isEntitySectionShown())
                    {
                        return false;
                    }

                    editor.focusedField = FocusedField::EntityAxis;
                    editor.entityPick.editingAxis = place;
                    editor.entityPick.pendingAxisText = std::to_string(
                        getCubeAxisOf(editor.entityPick.position, place));

                    return true;
                }},
            FamilyRow{
                .widgetAt = getEntityRowWidget,
                .placeCountOf = [](const Editor &editor)
                {
                    return std::min(
                        editor.entityList.rows.size(), kMaxEntityRows);
                },
                .hint = "one entity in the world - press again to aim the "
                        "camera at it",
                .activation = [](Editor &editor, const std::size_t place)
                { return editor.pressEntityRow(place); }},
            FamilyRow{
                .widgetAt = getMapRowWidget,
                .placeCountOf = [](const Editor &editor)
                {
                    return editor.fileChooser.fileDialog.has_value()
                               ? editor.fileChooser.folderEntries.size()
                                     + editor.fileChooser.mapEntries.size()
                               : std::size_t{0};
                },
                .activation = [](Editor &editor, const std::size_t place)
                {
                    auto &fileChooser = editor.fileChooser;

                    if (place < fileChooser.folderEntries.size())
                    {
                        fileChooser.fileDialog->folder =
                            (std::filesystem::path(
                                 fileChooser.fileDialog->folder)
                             / fileChooser.folderEntries.at(place))
                                .string();
                        fileChooser.listFolder(
                            fileChooser.fileDialog->folder);

                        return true;
                    }

                    fileChooser.fileDialog->fileName =
                        fileChooser.mapEntries.at(
                            place - fileChooser.folderEntries.size());

                    const auto pickedAgain =
                        editor.pointer.lastPickedWidget
                            == getMapRowWidget(place)
                        && editor.tick < editor.pointer.lastPickedAt + 30;

                    editor.pointer.lastPickedWidget = getMapRowWidget(place);
                    editor.pointer.lastPickedAt = editor.tick;

                    if (pickedAgain)
                    {
                        editor.confirmFileDialog();
                    }

                    return true;
                }}};

        static_assert(
            isEveryRowClaimed(kFamilyRows),
            "a family row neither activates nor names its delegate");

        static constexpr std::array kSliderRows{
            SliderRow{
                .widget = kFrequencyWidget,
                .decorNeed = true,
                .slideGate = [](const Editor &editor)
                { return editor.stroke.selectedTile.has_value(); },
                .valueOf = [](const Editor &editor)
                {
                    const auto *decor =
                        editor.stroke.selectedTile.has_value()
                            ? decor::decorOf(
                                  editor.document.map.decor,
                                  *editor.stroke.selectedTile)
                            : nullptr;

                    return decor == nullptr
                               ? std::uint32_t{0}
                               : static_cast<std::uint32_t>(
                                   decor->frequency);
                },
                .slideEffect =
                    [](Editor &editor, const std::uint32_t value)
                {
                    if (!editor.stroke.selectedTile.has_value())
                    {
                        return;
                    }

                    editor.document.map.decor =
                        decor::getWithFrequencySet(
                            editor.document.map.decor,
                            *editor.stroke.selectedTile,
                            static_cast<std::uint8_t>(value));
                },
                .settleEffect = [](Editor &editor)
                { editor.rebuildWorld(); }},
            SliderRow{
                .widget = kDecorWeightWidget,
                .decorNeed = true,
                .slideGate = [](const Editor &editor)
                { return editor.stroke.selectedTile.has_value(); },
                .valueOf = [](const Editor &editor)
                {
                    const auto *decor =
                        editor.stroke.selectedTile.has_value()
                            ? decor::decorOf(
                                  editor.document.map.decor,
                                  *editor.stroke.selectedTile)
                            : nullptr;

                    return decor == nullptr
                               ? std::uint32_t{0}
                               : static_cast<std::uint32_t>(
                                   decor->weight);
                },
                .slideEffect =
                    [](Editor &editor, const std::uint32_t value)
                {
                    if (!editor.stroke.selectedTile.has_value())
                    {
                        return;
                    }

                    editor.document.map.decor =
                        decor::getWithWeightSet(
                            editor.document.map.decor,
                            *editor.stroke.selectedTile,
                            static_cast<std::uint8_t>(value));
                },
                .settleEffect = [](Editor &editor)
                { editor.rebuildWorld(); }},
            SliderRow{
                .widget = kGlowWidget,
                .undoNeed = false,
                .slideGate = [](const Editor &editor)
                {
                    return editor.inkPanel.inkPicker.editingInk
                        .has_value();
                },
                .slideEffect =
                    [](Editor &editor, const std::uint32_t value)
                {
                    if (!editor.inkPanel.inkPicker.editingInk.has_value()
                        || *editor.inkPanel.inkPicker.editingInk
                               >= editor.document.map.glows.size())
                    {
                        return;
                    }

                    editor.document.map.glows.at(
                        *editor.inkPanel.inkPicker.editingInk) =
                        static_cast<std::uint8_t>(value);
                },
                .settleEffect = [](Editor &editor)
                { editor.atlasSheets.touch(); }},
            SliderRow{
                .widget = kAmbientWidget,
                .slideEffect =
                    [](Editor &editor, const std::uint32_t value)
                {
                    editor.document.map.ambient =
                        static_cast<std::uint8_t>(value);
                }},
            SliderRow{
                .widget = kVariantWeightWidget,
                .slideGate = [](const Editor &editor)
                { return editor.stroke.selectedTile.has_value(); },
                .valueOf = [](const Editor &editor)
                {
                    return static_cast<std::uint32_t>(
                        editor.variantWeightOf(
                            *editor.stroke.selectedTile));
                },
                .slideEffect =
                    [](Editor &editor, const std::uint32_t value)
                {
                    if (!editor.stroke.selectedTile.has_value())
                    {
                        return;
                    }

                    editor.document.map.familyGroups =
                        decor::getWithVariantWeightSet(
                            editor.document.map.familyGroups,
                            *editor.stroke.selectedTile,
                            static_cast<std::uint8_t>(value));
                },
                .settleEffect = [](Editor &editor)
                { editor.rebuildWorld(); }}};

        static_assert(
            widget::allDistinct(getRowWidgets(kSliderRows)),
            "two slider rows share a widget");

        static_assert(
            isEverySliderTagBacked(kSoloRows, kSliderRows),
            "a slider-tagged solo row has no slider row behind it");

        static constexpr std::array kFieldRows{
            FieldRow{
                .widget = kPickerNameWidget,
                .editEffect =
                    [](Editor &editor, const std::string &text)
                {
                    if (!editor.fileChooser.fileDialog.has_value()
                        || !editor.fileChooser.fileDialog->isSaveMode)
                    {
                        return;
                    }

                    editor.fileChooser.fileDialog->fileName = text;
                }},
            FieldRow{
                .widget = kInkHexWidget,
                .editEffect =
                    [](Editor &editor, const std::string &text)
                {
                    if (!editor.inkPanel.inkPicker.editingInk.has_value())
                    {
                        return;
                    }

                    editor.inkPanel.inkPicker.hexText = text;

                    const auto parsedColor =
                        getColorFromHex(editor.inkPanel.inkPicker.hexText);

                    if (parsedColor.has_value())
                    {
                        editor.inkPanel.recolorInk(*parsedColor);
                        editor.inkPanel.inkPicker.pickerHsv =
                            hsvOf(*parsedColor);
                    }
                }},
            FieldRow{
                .widget = kExitTargetWidget,
                .editEffect =
                    [](Editor &editor, const std::string &text)
                {
                    if (editor.focusedField != FocusedField::ExitTarget)
                    {
                        return;
                    }

                    editor.document.map.exitTarget = text;
                }},
            FieldRow{
                .widget = kCharacterNameWidget,
                .editEffect =
                    [](Editor &editor, const std::string &text)
                {
                    const auto chosenCharacter =
                        editor.worldView.characterTool().getChosenCharacter(
                            editor.document.map.characters.size());

                    if (editor.focusedField != FocusedField::CharacterName
                        || !chosenCharacter.has_value())
                    {
                        return;
                    }

                    editor.document.map.characters
                        .at(*chosenCharacter)
                        .name = text;
                }},
            FieldRow{
                .widget = kCharacterLineWidget,
                .editEffect =
                    [](Editor &editor, const std::string &text)
                {
                    if (editor.focusedField != FocusedField::CharacterLine)
                    {
                        return;
                    }

                    editor.worldView.characterTool().setPendingLine(text);
                }}};

        static_assert(
            widget::allDistinct(getRowWidgets(kFieldRows)),
            "two field rows share a widget");

        static constexpr std::array kFieldFamilyRows{
            Catalog::FieldFamilyRow{
                .widgetAt = [](const std::size_t place)
                {
                    return getComponentFieldWidget(
                        place / kMaxComponentFields,
                        place % kMaxComponentFields);
                },
                .placeCount = kMaxComponentSlots * kMaxComponentFields,
                .editEffect = [](
                    Editor &editor,
                    const std::size_t,
                    const std::string &text)
                {
                    if (editor.focusedField
                        != FocusedField::ComponentValue)
                    {
                        return;
                    }

                    editor.worldView.characterTool().setPendingValueText(
                        text);
                }},
            Catalog::FieldFamilyRow{
                .widgetAt = getMarkerFieldWidget,
                .placeCount = kMarkerAxisCount,
                .editEffect = [](
                    Editor &editor,
                    const std::size_t place,
                    const std::string &text)
                {
                    if (editor.focusedField != FocusedField::MarkerAxis
                        || editor.markerPick.editingAxis != place)
                    {
                        return;
                    }

                    editor.markerPick.pendingAxisText = text;
                }},
            Catalog::FieldFamilyRow{
                .widgetAt = getEntityFieldWidget,
                .placeCount = kMarkerAxisCount,
                .editEffect = [](
                    Editor &editor,
                    const std::size_t place,
                    const std::string &text)
                {
                    if (editor.focusedField != FocusedField::EntityAxis
                        || editor.entityPick.editingAxis != place)
                    {
                        return;
                    }

                    editor.entityPick.pendingAxisText = text;
                }}};

        static_assert(
            widget_catalog::isEveryFieldFamilyClaimed(kFieldFamilyRows),
            "a field family misses its widgets or its edit effect");

        static_assert(
            widget_catalog::isEveryFieldFamilyApart(
                kFieldFamilyRows, kFieldRows),
            "a field family widget stands on another field row");

        static constexpr auto kCatalog = Catalog{
            kSoloRows, kFamilyRows, kSliderRows, kFieldRows,
            kFieldFamilyRows};

        return kCatalog;
    }

}
