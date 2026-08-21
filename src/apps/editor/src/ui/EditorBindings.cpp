#include "antwika/editor/ui/EditorBindings.hpp"

namespace antwika::editor
{

    std::array<Action, kActionCount> allActions()
    {
        return {
            Action::Play,
            Action::PlayHere,
            Action::PlayApart,
            Action::Run,
            Action::Respawn,
            Action::Talk,
            Action::Eat,
            Action::Drink,
            Action::Save,
            Action::Load,
            Action::Fullscreen,
            Action::Undo,
            Action::Redo,
            Action::LevelUp,
            Action::LevelDown,
            Action::Corners,
            Action::WeaveLog,
            Action::Cancel,
            Action::ViewWorld,
            Action::ViewAtlases,
            Action::ViewCharacter,
            Action::ViewIcons,
            Action::ViewPlan,
            Action::ViewNext,
            Action::ViewBack,
            Action::ToolBrush,
            Action::ToolPicker,
            Action::ToolFreeLook,
            Action::ToolLighting,
            Action::ToolLamp,
            Action::ToolRuleLines,
            Action::ToolStart,
            Action::ToolExit,
            Action::ToolStamp,
            Action::ToolFigure,
            Action::ToolPlate,
            Action::KindStone,
            Action::KindWater,
            Action::KindRamp,
            Action::PaintBrush,
            Action::PaintLine,
            Action::PaintFill,
            Action::PaintSelect,
            Action::PaintRect,
            Action::PaintCircle,
            Action::ToggleBoundary,
            Action::ToggleForbidden,
            Action::Mirror,
            Action::Copy,
            Action::Cut,
            Action::Paste,
            Action::Delete,
            Action::WalkNorth,
            Action::WalkSouth,
            Action::WalkWest,
            Action::WalkEast,
            Action::WalkNorthAlt,
            Action::WalkSouthAlt,
            Action::WalkWestAlt,
            Action::WalkEastAlt};
    } // GCOVR_EXCL_LINE

    std::string_view actionLabel(const Action action)
    {
        switch (action)
        {
        case Action::Play:
            return "play the map";
        case Action::PlayHere:
            return "play from the pointer";
        case Action::PlayApart:
            return "play in a window of its own";
        case Action::Run:
            return "run";
        case Action::Respawn:
            return "back to the start";
        case Action::Talk:
            return "talk";
        case Action::Eat:
            return "eat what is carried";
        case Action::Drink:
            return "drink what is carried";
        case Action::Save:
            return "save the map";
        case Action::Load:
            return "load the map afresh";
        case Action::Fullscreen:
            return "fullscreen";
        case Action::Undo:
            return "undo";
        case Action::Redo:
            return "redo";
        case Action::LevelUp:
            return "a level up";
        case Action::LevelDown:
            return "a level down";
        case Action::Corners:
            return "corners joined";
        case Action::WeaveLog:
            return "log the weave";
        case Action::Cancel:
            return "let go of things";
        case Action::ViewWorld:
            return "world view";
        case Action::ViewAtlases:
            return "atlases view";
        case Action::ViewCharacter:
            return "character view";
        case Action::ViewIcons:
            return "icons view";
        case Action::ViewPlan:
            return "plan view";
        case Action::ViewNext:
            return "next view";
        case Action::ViewBack:
            return "view before";
        case Action::ToolBrush:
            return "brush tool";
        case Action::ToolPicker:
            return "picker tool";
        case Action::ToolFreeLook:
            return "free look";
        case Action::ToolLighting:
            return "lighting";
        case Action::ToolLamp:
            return "lamp tool";
        case Action::ToolRuleLines:
            return "rule lines";
        case Action::ToolStart:
            return "start tool";
        case Action::ToolExit:
            return "exit tool";
        case Action::ToolStamp:
            return "stamp tool";
        case Action::ToolFigure:
            return "figure tool";
        case Action::ToolPlate:
            return "plate tool";
        case Action::KindStone:
            return "stone cubes";
        case Action::KindWater:
            return "water cubes";
        case Action::KindRamp:
            return "ramp cubes";
        case Action::PaintBrush:
            return "pixel brush";
        case Action::PaintLine:
            return "pixel line";
        case Action::PaintFill:
            return "pixel fill";
        case Action::PaintSelect:
            return "pixel mark";
        case Action::PaintRect:
            return "pixel rectangle";
        case Action::PaintCircle:
            return "pixel circle";
        case Action::ToggleBoundary:
            return "edge rims";
        case Action::ToggleForbidden:
            return "edge shuts";
        case Action::Mirror:
            return "flip the mark";
        case Action::Copy:
            return "copy the mark";
        case Action::Cut:
            return "cut the mark";
        case Action::Paste:
            return "paste the mark";
        case Action::Delete:
            return "delete the mark";
        case Action::WalkNorth:
            return "walk north";
        case Action::WalkSouth:
            return "walk south";
        case Action::WalkWest:
            return "walk west";
        case Action::WalkEast:
            return "walk east";
        case Action::WalkNorthAlt:
            return "walk north too";
        case Action::WalkSouthAlt:
            return "walk south too";
        case Action::WalkWestAlt:
            return "walk west too";
        case Action::WalkEastAlt:
            break;
        }

        return "walk east too";
    }

    std::string_view actionKey(const Action action)
    {
        switch (action)
        {
        case Action::Play:
            return "play";
        case Action::PlayHere:
            return "playHere";
        case Action::PlayApart:
            return "playApart";
        case Action::Run:
            return "run";
        case Action::Respawn:
            return "respawn";
        case Action::Talk:
            return "talk";
        case Action::Eat:
            return "eat";
        case Action::Drink:
            return "drink";
        case Action::Save:
            return "save";
        case Action::Load:
            return "load";
        case Action::Fullscreen:
            return "fullscreen";
        case Action::Undo:
            return "undo";
        case Action::Redo:
            return "redo";
        case Action::LevelUp:
            return "levelUp";
        case Action::LevelDown:
            return "levelDown";
        case Action::Corners:
            return "corners";
        case Action::WeaveLog:
            return "weaveLog";
        case Action::Cancel:
            return "cancel";
        case Action::ViewWorld:
            return "viewWorld";
        case Action::ViewAtlases:
            return "viewAtlases";
        case Action::ViewCharacter:
            return "viewCharacter";
        case Action::ViewIcons:
            return "viewIcons";
        case Action::ViewPlan:
            return "viewPlan";
        case Action::ViewNext:
            return "viewNext";
        case Action::ViewBack:
            return "viewBack";
        case Action::ToolBrush:
            return "toolBrush";
        case Action::ToolPicker:
            return "toolPicker";
        case Action::ToolFreeLook:
            return "toolEye";
        case Action::ToolLighting:
            return "toolLight";
        case Action::ToolLamp:
            return "toolLamp";
        case Action::ToolRuleLines:
            return "toolTies";
        case Action::ToolStart:
            return "toolStart";
        case Action::ToolExit:
            return "toolExit";
        case Action::ToolStamp:
            return "toolStamp";
        case Action::ToolFigure:
            return "toolFigure";
        case Action::ToolPlate:
            return "toolPlate";
        case Action::KindStone:
            return "kindStone";
        case Action::KindWater:
            return "kindWater";
        case Action::KindRamp:
            return "kindRamp";
        case Action::PaintBrush:
            return "paintBrush";
        case Action::PaintLine:
            return "paintLine";
        case Action::PaintFill:
            return "paintFill";
        case Action::PaintSelect:
            return "paintMark";
        case Action::PaintRect:
            return "paintRect";
        case Action::PaintCircle:
            return "paintCircle";
        case Action::ToggleBoundary:
            return "rim";
        case Action::ToggleForbidden:
            return "shut";
        case Action::Mirror:
            return "flip";
        case Action::Copy:
            return "copy";
        case Action::Cut:
            return "cut";
        case Action::Paste:
            return "paste";
        case Action::Delete:
            return "delete";
        case Action::WalkNorth:
            return "walkNorth";
        case Action::WalkSouth:
            return "walkSouth";
        case Action::WalkWest:
            return "walkWest";
        case Action::WalkEast:
            return "walkEast";
        case Action::WalkNorthAlt:
            return "walkNorthToo";
        case Action::WalkSouthAlt:
            return "walkSouthToo";
        case Action::WalkWestAlt:
            return "walkWestToo";
        case Action::WalkEastAlt:
            break;
        }

        return "walkEastToo";
    }

    ui::WidgetId keyRowWidget(const std::size_t rowIndex)
    {
        return ui::WidgetId{
            300 + static_cast<std::uint64_t>(rowIndex)};
    }

    KeyBindings defaultChords()
    {
        KeyBindings bindings;

        bindings[Action::Play] = Chord{.key = input::Key::F5};
        bindings[Action::PlayHere] =
            Chord{.key = input::Key::F5, .shift = true};
        bindings[Action::PlayApart] = Chord{.key = input::Key::F6};
        bindings[Action::Run] = Chord{.key = input::Key::Space};
        bindings[Action::Respawn] = Chord{.key = input::Key::Home};
        bindings[Action::Talk] = Chord{.key = input::Key::E};
        bindings[Action::Eat] = Chord{.key = input::Key::F};
        bindings[Action::Drink] = Chord{.key = input::Key::G};
        bindings[Action::Save] = Chord{.key = input::Key::S, .ctrl = true};
        bindings[Action::Load] = Chord{.key = input::Key::F9};
        bindings[Action::Fullscreen] = Chord{.key = input::Key::F10};
        bindings[Action::Undo] = Chord{.key = input::Key::Z, .ctrl = true};
        bindings[Action::Redo] =
            Chord{.key = input::Key::Z, .ctrl = true, .shift = true};
        bindings[Action::LevelUp] = Chord{.key = input::Key::PageUp};
        bindings[Action::LevelDown] = Chord{.key = input::Key::PageDown};
        bindings[Action::Corners] = Chord{.key = input::Key::C};
        bindings[Action::WeaveLog] = Chord{.key = input::Key::G};
        bindings[Action::Cancel] = Chord{.key = input::Key::Escape};
        bindings[Action::ViewWorld] = Chord{.key = input::Key::Digit1};
        bindings[Action::ViewAtlases] = Chord{.key = input::Key::Digit2};
        bindings[Action::ViewCharacter] = Chord{.key = input::Key::Digit3};
        bindings[Action::ViewIcons] = Chord{.key = input::Key::Digit4};
        bindings[Action::ViewPlan] = Chord{.key = input::Key::Digit5};
        bindings[Action::ViewNext] = Chord{.key = input::Key::Tab};
        bindings[Action::ViewBack] =
            Chord{.key = input::Key::Tab, .shift = true};
        bindings[Action::ToolBrush] = Chord{.key = input::Key::B};
        bindings[Action::ToolPicker] = Chord{.key = input::Key::I};
        bindings[Action::ToolFreeLook] =
            Chord{.key = input::Key::F, .shift = true};
        bindings[Action::ToolLighting] = Chord{.key = input::Key::L};
        bindings[Action::ToolLamp] = std::nullopt;
        bindings[Action::ToolRuleLines] = Chord{.key = input::Key::T};
        bindings[Action::ToolStart] = std::nullopt;
        bindings[Action::ToolExit] = std::nullopt;
        bindings[Action::ToolStamp] = std::nullopt;
        bindings[Action::ToolFigure] = std::nullopt;
        bindings[Action::ToolPlate] = std::nullopt;
        bindings[Action::KindStone] = Chord{.key = input::Key::N};
        bindings[Action::KindWater] = std::nullopt;
        bindings[Action::KindRamp] = Chord{.key = input::Key::R};
        bindings[Action::PaintBrush] = Chord{.key = input::Key::B};
        bindings[Action::PaintLine] = Chord{.key = input::Key::L};
        bindings[Action::PaintFill] = Chord{.key = input::Key::F};
        bindings[Action::PaintSelect] = Chord{.key = input::Key::M};
        bindings[Action::PaintRect] = std::nullopt;
        bindings[Action::PaintCircle] = std::nullopt;
        bindings[Action::ToggleBoundary] = Chord{.key = input::Key::R};
        bindings[Action::ToggleForbidden] = Chord{.key = input::Key::X};
        bindings[Action::Mirror] = Chord{.key = input::Key::H};
        bindings[Action::Copy] = Chord{.key = input::Key::C, .ctrl = true};
        bindings[Action::Cut] = Chord{.key = input::Key::X, .ctrl = true};
        bindings[Action::Paste] = Chord{.key = input::Key::V, .ctrl = true};
        bindings[Action::Delete] = Chord{.key = input::Key::Delete};
        bindings[Action::WalkNorth] = Chord{.key = input::Key::W};
        bindings[Action::WalkSouth] = Chord{.key = input::Key::S};
        bindings[Action::WalkWest] = Chord{.key = input::Key::A};
        bindings[Action::WalkEast] = Chord{.key = input::Key::D};
        bindings[Action::WalkNorthAlt] = Chord{.key = input::Key::ArrowUp};
        bindings[Action::WalkSouthAlt] = Chord{.key = input::Key::ArrowDown};
        bindings[Action::WalkWestAlt] = Chord{.key = input::Key::ArrowLeft};
        bindings[Action::WalkEastAlt] = Chord{.key = input::Key::ArrowRight};

        return bindings;
    } // GCOVR_EXCL_LINE

    std::string heldAction(const Action action)
    {
        return std::string(actionKey(action)) + ".held";
    }

    std::string shiftedAction(const Action action)
    {
        return std::string(actionKey(action)) + ".shift";
    }

    input::ActionMap actionMapFrom(const KeyBindings &keyBindings)
    {
        input::ActionMap actions;

        for (const auto &[action, chord] : keyBindings)
        {
            if (!chord.has_value())
            {
                continue;
            }

            actions.bind(
                std::string(actionKey(action)),
                chord->key,
                input::KeyModifiers{
                    .shift = chord->shift,
                    .control = chord->ctrl,
                    .alt = chord->alt},
                input::ExtraModifiers::Refused);

            actions.bind(heldAction(action), chord->key);

            if (!chord->shift)
            {
                actions.bind(
                    shiftedAction(action),
                    chord->key,
                    input::KeyModifiers{.shift = true},
                    input::ExtraModifiers::Refused);
            }
        }

        return actions;
    }

}
