#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/input/ActionMap.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/widget/WidgetId.hpp>

namespace antwika::editor
{

    enum class Action : std::uint8_t
    {
        Play,
        PlayHere,
        PlayApart,
        Run,
        Respawn,
        Talk,
        Save,
        Load,
        Fullscreen,
        Undo,
        Redo,
        LevelUp,
        LevelDown,
        Corners,
        FreeLook,
        EditorLighting,
        RuleLines,
        WeaveLog,
        Cancel,
        ViewWorld,
        ViewAtlases,
        ViewCharacter,
        ViewIcons,
        ViewPlan,
        ViewGizmos,
        ViewNext,
        ViewBack,
        ToolSelect,
        ToolPicker,
        ToolLamp,
        ToolStart,
        ToolExit,
        ToolStamp,
        ToolCharacter,
        KindStone,
        KindWater,
        KindRamp,
        PaintBrush,
        PaintLine,
        PaintFill,
        PaintSelect,
        PaintRect,
        PaintCircle,
        ToggleBoundary,
        ToggleForbidden,
        Mirror,
        Copy,
        Cut,
        Paste,
        Delete,
        Eat,
        Drink,
        WalkNorth,
        WalkSouth,
        WalkWest,
        WalkEast,
        WalkNorthAlt,
        WalkSouthAlt,
        WalkWestAlt,
        WalkEastAlt,
    };

    [[nodiscard]] constexpr Action getLastEnumerator(Action) noexcept
    {
        return Action::WalkEastAlt;
    }

    inline constexpr std::size_t kActionCount =
        enums::kCount<Action>;

    [[nodiscard]] std::array<Action, kActionCount> getAllActions();

    struct Chord final
    {
        input::Key key = input::Key::A;

        bool ctrl = false;

        bool shift = false;

        bool alt = false;

        [[nodiscard]] bool operator==(const Chord &other) const
            = default;
    };

    using KeyBindings = std::map<Action, std::optional<Chord>>;

    [[nodiscard]] std::string_view getActionLabel(Action action);

    [[nodiscard]] std::string_view getActionKey(Action action);

    [[nodiscard]] KeyBindings getDefaultChords();

    [[nodiscard]] std::string getHeldAction(Action action);

    [[nodiscard]] std::string getShiftedAction(Action action);

    [[nodiscard]] input::ActionMap actionMapFrom(
        const KeyBindings &keyBindings);

    [[nodiscard]] std::string_view getKeyName(input::Key key);

    [[nodiscard]] std::string getChordName(
        const std::optional<Chord> &chord);

    void saveChords(
        const KeyBindings &keyBindings, const std::string &path);

    [[nodiscard]] KeyBindings getLoadChords(const std::string &path);

    [[nodiscard]] widget::WidgetId getKeyRowWidget(std::size_t rowIndex);

}
