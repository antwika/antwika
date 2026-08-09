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

    class LocaleSource final : public ITickEventSource
    {
    public:
        LocaleSource(
            ITickEventSource &inner,
            std::optional<Locale> announced) noexcept;

        LocaleSource(const LocaleSource &) = delete;
        LocaleSource(LocaleSource &&) = delete;

        LocaleSource &operator=(const LocaleSource &) = delete;
        LocaleSource &operator=(LocaleSource &&) = delete;

        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        std::optional<Locale> announced;
    };

}
