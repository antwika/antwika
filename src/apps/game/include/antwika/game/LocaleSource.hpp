#pragma once

#include <optional>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::game
{

    using antwika::event::Event;
    using antwika::event::ITickEventSource;
    using antwika::i18n::Locale;

    /**
     * @brief Puts the machine's own language on the wire, once, at the
     * start of a live run.
     *
     * **This is the seam that keeps a picked language from breaking a
     * replay**, and it is BindingSource's shape exactly, for the same
     * reason and with the same three properties. A language read off the
     * player's options file is externally supplied and is not derivable
     * from anything else in the run, so the cross-module rule that only
     * externally-supplied input is persisted says outright that it has
     * to be recorded. Announcing it here, upstream of
     * event::TickEventRecorder, is what records it: the recorder writes
     * unconditionally, so a --record file carries the language the
     * session was played in and a replay describes the same strings,
     * lays out the same widths and resolves every recorded click to the
     * widget it hit live.
     *
     * **A replay announces nothing**, because the recording already
     * holds the announcement. Constructed with no locale at all, it is a
     * pure pass-through and the events it hands on are the file's,
     * unchanged.
     *
     * **A locale equal to kDefaultLocale is not announced.** A run on a
     * machine that has never picked a language therefore records exactly
     * what it recorded before this class existed, and every replay
     * written before it still means what it meant.
     *
     * The announcement goes ahead of the tick's own events rather than
     * after them, so the first click of a session is already resolved
     * against the language the session was played in.
     */
    class LocaleSource final : public ITickEventSource
    {
    public:
        /**
         * @brief Construct the announcer over what it wraps.
         * @param inner The source whose events pass through; must
         * outlive this object.
         * @param announced The language the machine is carrying, or
         * nothing for a run whose input comes from a file and which
         * therefore already holds one.
         */
        LocaleSource(
            ITickEventSource &inner,
            std::optional<Locale> announced) noexcept;

        LocaleSource(const LocaleSource &) = delete;
        LocaleSource(LocaleSource &&) = delete;

        LocaleSource &operator=(const LocaleSource &) = delete;
        LocaleSource &operator=(LocaleSource &&) = delete;

        /**
         * @brief Get a tick's events, announcing the machine's language
         * ahead of the first tick's.
         * @param tick The tick to fetch events for.
         * @return The wrapped source's events, with a game.set_locale
         * ahead of them when the machine is carrying a language other
         * than the default, on the first call only.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        std::optional<Locale> announced;
    };

} // namespace antwika::game
