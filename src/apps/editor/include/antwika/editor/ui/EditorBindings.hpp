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
#include <antwika/ui/WidgetId.hpp>

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
        WeaveLog,
        Cancel,
        ViewWorld,
        ViewAtlases,
        ViewCharacter,
        ViewIcons,
        ViewPlan,
        ViewNext,
        ViewBack,
        ToolBrush,
        ToolPicker,
        ToolFreeLook,
        ToolLighting,
        ToolLamp,
        ToolRuleLines,
        ToolStart,
        ToolExit,
        ToolStamp,
        ToolFigure,
        ToolPlate,
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

    [[nodiscard]] constexpr Action lastEnumerator(Action) noexcept
    {
        return Action::WalkEastAlt;
    }

    inline constexpr std::size_t kActionCount =
        enums::kCount<Action>;

    [[nodiscard]] std::array<Action, kActionCount> allActions();

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

    [[nodiscard]] std::string_view actionLabel(Action action);

    [[nodiscard]] std::string_view actionKey(Action action);

    [[nodiscard]] KeyBindings defaultChords();

    [[nodiscard]] std::string heldAction(Action action);

    [[nodiscard]] std::string shiftedAction(Action action);

    [[nodiscard]] input::ActionMap actionMapFrom(
        const KeyBindings &keyBindings);

    [[nodiscard]] std::string_view keyName(input::Key key);

    [[nodiscard]] std::string chordName(
        const std::optional<Chord> &chord);

    void saveChords(
        const KeyBindings &keyBindings, const std::string &path);

    [[nodiscard]] KeyBindings loadChords(const std::string &path);

    [[nodiscard]] ui::WidgetId keyRowWidget(std::size_t rowIndex);

    inline constexpr ui::WidgetId kKeysDoneWidget{380};

    inline constexpr ui::WidgetId kKeysResetWidget{381};

}
