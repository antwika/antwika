#pragma once

#include <cstddef>

#include <antwika/gfx/Size.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/console/ConsoleState.hpp"

namespace antwika::console
{

    using antwika::gfx::Size;
    using antwika::ui::Frame;
    using antwika::ui::Keyboard;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;

    namespace consoleWidgets
    {
        /**
         * @brief The sheet the console drops down as.
         */
        inline constexpr WidgetId kSheet{401};

        /**
         * @brief The field a command is typed into.
         */
        inline constexpr WidgetId kInput{402};
    } // namespace consoleWidgets

    /**
     * @brief How many history lines the open console lists.
     *
     * The newest lines win, since the line a command just answered
     * with is the one whoever typed it is reading.
     */
    inline constexpr std::size_t kConsoleHistoryShown = 8;

    /**
     * @brief Turns the console's state into the sheet over the city.
     *
     * Stateless and deterministic, like Toolbar and SaveLoadScene: the
     * same canvas, pointer, keyboard and ConsoleState always produce
     * the same picture and the same answer about what was typed.
     *
     * The sheet spans the canvas and stands state.height() tall, so
     * the slide the tween shapes is entirely the state's doing; part
     * way along it is an empty sheet, and only a console standing
     * fully open lists its history and offers the field -- which is
     * how "the input reads only fully open" looks on screen.
     *
     * **Every word it draws is a literal rather than a MessageId.**
     * A command language is a format, like a save file's kind names:
     * `dump_state` is what the command is called in every language,
     * and history lines are state a dump carries, so wording them per
     * locale would make the dumped console a function of the language
     * it was typed under.
     */
    class ConsoleScene final
    {
    public:
        /**
         * @brief Describe the console over one canvas.
         * @param canvas The area the console drops down over.
         * @param pointer The recorded pointer, for the field's press.
         * @param keyboard This tick's key edges, already worded for
         * antwika::ui by the sink.
         * @param state The console being described.
         * @return The frame: the picture, and what the input produced.
         */
        [[nodiscard]] Frame describe(
            Size canvas,
            Pointer pointer,
            const Keyboard &keyboard,
            const ConsoleState &state) const;
    };

} // namespace antwika::console
