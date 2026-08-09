#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

#include "antwika/game/AppMode.hpp"

namespace antwika::game
{

    class ModeGatedSink final : public ITickEventSink
    {
    public:
        ModeGatedSink(
            ITickEventSink &inner,
            const AppModeState &mode,
            AppMode active) noexcept;

        ModeGatedSink(const ModeGatedSink &) = delete;
        ModeGatedSink(ModeGatedSink &&) = delete;

        ModeGatedSink &operator=(const ModeGatedSink &) = delete;
        ModeGatedSink &operator=(ModeGatedSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        ITickEventSink &inner;
        const AppModeState &mode;
        AppMode active;
    };

}
