#pragma once

#include <string>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/ui/Interactions.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/console/ConsolePicture.hpp"
#include "antwika/console/ConsoleScene.hpp"
#include "antwika/console/ConsoleState.hpp"
#include "antwika/console/IConsoleCommands.hpp"
#include "antwika/console/IConsoleControls.hpp"
#include "antwika/console/InputFold.hpp"

namespace antwika::console
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::ui::Interactions;
    using antwika::ui::Keyboard;
    using antwika::ui::Pointer;

    /**
     * @brief Everything the console sink drives, named one per field.
     *
     * A struct with designated initialisers rather than a parameter
     * list, for the reason game::GameWiring gives: a row of
     * same-typed positional references is exactly how two of them end
     * up swapped.
     *
     * Every reference is borrowed and must outlive the ConsoleSink.
     */
    struct ConsoleSinkSetup
    {
        /** @brief The console being driven. */
        ConsoleState &console;

        /** @brief The folded input, registered ahead of this sink. */
        const InputFold &input;

        /** @brief The console's own picture, written every tick. */
        ConsolePicture &picture;

        /** @brief Describes the console. */
        const ConsoleScene &scene;

        /** @brief The toggle and execute keys, and the typing board. */
        const IConsoleControls &controls;

        /** @brief What each executed line does. */
        IConsoleCommands &commands;
    };

    /**
     * @brief Turns this tick's input into the console's slide, its
     * typing and its commands, and the console into a picture.
     *
     * **The console defines no event of its own.** The toggle key, the
     * typing and the execute press are the input; they are resolved
     * against the controls and the console's own state here, inside
     * the tick path and downstream of the recorder, and the slide, the
     * history and each command's effect are all regenerated from them
     * on replay.
     * No `console.*` event name may ever exist.
     *
     * Register it ahead of every sink that reads a key or a pixel,
     * each of those wrapped in a ConsoleGatedSink -- the console is on
     * top, so what it stands over it takes, and it belongs to no mode:
     * a debugging surface has to be reachable from whichever screen
     * the thing being debugged is on.
     *
     * What an executed line *does* is IConsoleCommands', so this sink
     * knows no command by name; SnapshotCommands is the pair every
     * application mounts.
     */
    class ConsoleSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over everything it drives.
         * @param setup The collaborators, each of which must outlive
         * this sink.
         */
        explicit ConsoleSink(const ConsoleSinkSetup &setup);

        ConsoleSink(const ConsoleSink &) = delete;
        ConsoleSink(ConsoleSink &&) = delete;

        ConsoleSink &operator=(const ConsoleSink &) = delete;
        ConsoleSink &operator=(ConsoleSink &&) = delete;

        /**
         * @brief Apply a tick event.
         * @param event engine.tick advances the slide and re-describes
         * the picture; an input.* event is offered to the toggle key
         * and then, fully open, to the field; anything else is
         * ignored.
         * @throws SnapshotError If a command's own work cannot be
         * done and its policy is to end the run -- see
         * SnapshotCommands on a dump that cannot write.
         */
        void handle(const TickEvent &event) override;

    private:
        [[nodiscard]] Pointer pointerNow(bool pressed) const;

        void refreshAndAct(bool pressed, const Keyboard &keyboard);

        void act(const Interactions &interactions);

        void execute(const std::string &command);

        ConsoleSinkSetup setup;
    };

} // namespace antwika::console
