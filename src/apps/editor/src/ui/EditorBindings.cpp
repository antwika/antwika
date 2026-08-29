#include "antwika/editor/ui/EditorBindings.hpp"

#include "antwika/editor/ui/WidgetIds.hpp"

namespace antwika::editor
{

    namespace
    {
        struct ActionRow final
        {
            Action action;
            std::string_view key;
            std::string_view label;
            std::optional<Chord> chord;
        };

        constexpr std::array<ActionRow, kActionCount> kActionRows{{
            {Action::Play,
             "play",
             "play the map",
             Chord{.key = input::Key::F5}},
            {Action::PlayHere,
             "playHere",
             "play from the pointer",
             Chord{.key = input::Key::F5, .shift = true}},
            {Action::PlayApart,
             "playApart",
             "play in a window of its own",
             Chord{.key = input::Key::F6}},
            {Action::Run, "run", "run", Chord{.key = input::Key::Space}},
            {Action::Respawn,
             "respawn",
             "back to the start",
             Chord{.key = input::Key::Home}},
            {Action::Talk, "talk", "talk", Chord{.key = input::Key::E}},
            {Action::Save,
             "save",
             "save the map",
             Chord{.key = input::Key::S, .ctrl = true}},
            {Action::Load,
             "load",
             "load the map afresh",
             Chord{.key = input::Key::F9}},
            {Action::Fullscreen,
             "fullscreen",
             "fullscreen",
             Chord{.key = input::Key::F10}},
            {Action::Undo,
             "undo",
             "undo",
             Chord{.key = input::Key::Z, .ctrl = true}},
            {Action::Redo,
             "redo",
             "redo",
             Chord{.key = input::Key::Z, .ctrl = true, .shift = true}},
            {Action::LevelUp,
             "levelUp",
             "a level up",
             Chord{.key = input::Key::PageUp}},
            {Action::LevelDown,
             "levelDown",
             "a level down",
             Chord{.key = input::Key::PageDown}},
            {Action::Corners,
             "corners",
             "corners joined",
             Chord{.key = input::Key::C}},
            {Action::FreeLook,
             "toolEye",
             "free look",
             Chord{.key = input::Key::F, .shift = true}},
            {Action::EditorLighting,
             "toolLight",
             "editor lighting",
             Chord{.key = input::Key::L}},
            {Action::RuleLines,
             "toolTies",
             "rule lines",
             Chord{.key = input::Key::T}},
            {Action::WeaveLog,
             "weaveLog",
             "log the weave",
             Chord{.key = input::Key::G}},
            {Action::Cancel,
             "cancel",
             "let go of things",
             Chord{.key = input::Key::Escape}},
            {Action::ViewWorld,
             "viewWorld",
             "world view",
             Chord{.key = input::Key::Digit1}},
            {Action::ViewAtlases,
             "viewAtlases",
             "atlases view",
             Chord{.key = input::Key::Digit2}},
            {Action::ViewCharacter,
             "viewCharacter",
             "character view",
             Chord{.key = input::Key::Digit3}},
            {Action::ViewIcons,
             "viewIcons",
             "icons view",
             Chord{.key = input::Key::Digit4}},
            {Action::ViewPlan,
             "viewPlan",
             "plan view",
             Chord{.key = input::Key::Digit5}},
            {Action::ViewGizmos,
             "viewGizmos",
             "gizmos view",
             Chord{.key = input::Key::Digit6}},
            {Action::ViewNext,
             "viewNext",
             "next view",
             Chord{.key = input::Key::Tab}},
            {Action::ViewBack,
             "viewBack",
             "view before",
             Chord{.key = input::Key::Tab, .shift = true}},
            {Action::ToolSelect,
             "toolSelect",
             "select tool",
             Chord{.key = input::Key::V}},
            {Action::ToolPicker,
             "toolPicker",
             "picker tool",
             Chord{.key = input::Key::I}},
            {Action::ToolLamp, "toolLamp", "lamp tool", std::nullopt},
            {Action::ToolStart, "toolStart", "start tool", std::nullopt},
            {Action::ToolExit, "toolExit", "exit tool", std::nullopt},
            {Action::ToolStamp, "toolStamp", "stamp tool", std::nullopt},
            {Action::ToolCharacter,
             "toolCharacter",
             "character tool",
             std::nullopt},
            {Action::KindStone,
             "kindStone",
             "stone cubes",
             Chord{.key = input::Key::N}},
            {Action::KindWater, "kindWater", "water cubes", std::nullopt},
            {Action::KindRamp,
             "kindRamp",
             "ramp cubes",
             Chord{.key = input::Key::R}},
            {Action::PaintBrush,
             "paintBrush",
             "pixel brush",
             Chord{.key = input::Key::B}},
            {Action::PaintLine,
             "paintLine",
             "pixel line",
             Chord{.key = input::Key::L}},
            {Action::PaintFill,
             "paintFill",
             "pixel fill",
             Chord{.key = input::Key::F}},
            {Action::PaintSelect,
             "paintMark",
             "pixel mark",
             Chord{.key = input::Key::M}},
            {Action::PaintRect, "paintRect", "pixel rectangle", std::nullopt},
            {Action::PaintCircle, "paintCircle", "pixel circle", std::nullopt},
            {Action::ToggleBoundary,
             "rim",
             "edge rims",
             Chord{.key = input::Key::R}},
            {Action::ToggleForbidden,
             "shut",
             "edge shuts",
             Chord{.key = input::Key::X}},
            {Action::Mirror,
             "flip",
             "flip the mark",
             Chord{.key = input::Key::H}},
            {Action::Copy,
             "copy",
             "copy the mark",
             Chord{.key = input::Key::C, .ctrl = true}},
            {Action::Cut,
             "cut",
             "cut the mark",
             Chord{.key = input::Key::X, .ctrl = true}},
            {Action::Paste,
             "paste",
             "paste the mark",
             Chord{.key = input::Key::V, .ctrl = true}},
            {Action::Delete,
             "delete",
             "delete the mark",
             Chord{.key = input::Key::Delete}},
            {Action::Eat,
             "eat",
             "eat what is carried",
             Chord{.key = input::Key::F}},
            {Action::Drink,
             "drink",
             "drink what is carried",
             Chord{.key = input::Key::G}},
            {Action::WalkNorth,
             "walkNorth",
             "walk north",
             Chord{.key = input::Key::W}},
            {Action::WalkSouth,
             "walkSouth",
             "walk south",
             Chord{.key = input::Key::S}},
            {Action::WalkWest,
             "walkWest",
             "walk west",
             Chord{.key = input::Key::A}},
            {Action::WalkEast,
             "walkEast",
             "walk east",
             Chord{.key = input::Key::D}},
            {Action::WalkNorthAlt,
             "walkNorthToo",
             "walk north too",
             Chord{.key = input::Key::ArrowUp}},
            {Action::WalkSouthAlt,
             "walkSouthToo",
             "walk south too",
             Chord{.key = input::Key::ArrowDown}},
            {Action::WalkWestAlt,
             "walkWestToo",
             "walk west too",
             Chord{.key = input::Key::ArrowLeft}},
            {Action::WalkEastAlt,
             "walkEastToo",
             "walk east too",
             Chord{.key = input::Key::ArrowRight}}}};

        static_assert(enums::tagsInOrder(kActionRows, &ActionRow::action));
    }

    std::array<Action, kActionCount> getAllActions()
    {
        return enums::kAll<Action>;
    } // GCOVR_EXCL_LINE

    std::string_view getActionLabel(const Action action)
    {
        return enums::lookup(kActionRows, action).label;
    }

    std::string_view getActionKey(const Action action)
    {
        return enums::lookup(kActionRows, action).key;
    }

    widget::WidgetId getKeyRowWidget(const std::size_t rowIndex)
    {
        return getWidgetAfter(kFirstKeyRowWidget, rowIndex);
    }

    KeyBindings getDefaultChords()
    {
        KeyBindings bindings;

        for (const auto &row : kActionRows)
        {
            bindings[row.action] = row.chord;
        }

        return bindings;
    } // GCOVR_EXCL_LINE

    std::string getHeldAction(const Action action)
    {
        return std::string(getActionKey(action)) + ".held";
    }

    std::string getShiftedAction(const Action action)
    {
        return std::string(getActionKey(action)) + ".shift";
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
                std::string(getActionKey(action)),
                chord->key,
                input::KeyModifiers{
                    .shift = chord->shift,
                    .control = chord->ctrl,
                    .alt = chord->alt},
                input::ExtraModifiers::Refused);

            actions.bind(getHeldAction(action), chord->key);

            if (!chord->shift)
            {
                actions.bind(
                    getShiftedAction(action),
                    chord->key,
                    input::KeyModifiers{.shift = true},
                    input::ExtraModifiers::Refused);
            }
        }

        return actions;
    }

}
