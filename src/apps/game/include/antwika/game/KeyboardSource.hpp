#pragma once

#include <optional>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/KeyboardLayout.hpp"

namespace antwika::game
{

    using antwika::event::Event;
    using antwika::event::ITickEventSource;

    /**
     * @brief Puts the machine's own keyboard layout on the wire, once,
     * at the start of a live run.
     *
     * LocaleSource's shape exactly, for its reason exactly: what a key
     * press types is a function of the layout, the machine's layout
     * comes off the options file, and no replayed input implies it --
     * so a --record file has to carry it, and announcing it upstream
     * of the recorder is what records it.
     *
     * **A replay announces nothing**, because the recording already
     * holds the announcement.
     * **A layout equal to kDefaultKeyboardLayout is not announced**,
     * so a machine that never picked one records exactly what it
     * recorded before this class existed.
     */
    class KeyboardSource final : public ITickEventSource
    {
    public:
        /**
         * @brief Construct the announcer over what it wraps.
         * @param inner The source whose events pass through; must
         * outlive this object.
         * @param announced The layout the machine is carrying, or
         * nothing for a run whose input comes from a file and which
         * therefore already holds one.
         */
        KeyboardSource(
            ITickEventSource &inner,
            std::optional<KeyboardLayout> announced) noexcept;

        KeyboardSource(const KeyboardSource &) = delete;
        KeyboardSource(KeyboardSource &&) = delete;

        KeyboardSource &operator=(const KeyboardSource &) = delete;
        KeyboardSource &operator=(KeyboardSource &&) = delete;

        /**
         * @brief Get a tick's events, announcing the machine's layout
         * ahead of the first tick's.
         * @param tick The tick to fetch events for.
         * @return The wrapped source's events, with a
         * game.set_keyboard ahead of them when the machine carries a
         * layout other than the default, on the first call only.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        std::optional<KeyboardLayout> announced;
    };

} // namespace antwika::game
