#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

#include "antwika/game/AppMode.hpp"

namespace antwika::game
{

    /**
     * @brief Hands a sink this tick's input only while the app is in one
     * mode.
     *
     * **Input is what a mode gates, and only input.** engine.tick always
     * reaches the wrapped sink, because per-tick work -- committing the
     * world, running the systems, describing the picture the renderer is
     * about to paint -- has to happen in every mode, and a tick that
     * stopped arriving would stop the renderer and the pacer with it.
     * What a mode changes is what a *click* means, which is exactly the
     * input.
     *
     * A decorator rather than a mode check inside each sink, so that a
     * sink stays about its own subject and a new mode is a wiring change
     * in bootstrap() rather than an edit spread across every sink.
     *
     * It is downstream of the recorder like everything else here: the
     * mode it reads is regenerated from the recorded input, so a replay
     * gates exactly the same events.
     */
    class ModeGatedSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the gate over the sink it guards.
         * @param inner The sink being guarded. Must outlive this gate.
         * @param mode The app's mode. Must outlive this gate, and must be
         * registered ahead of it so that a change staged last tick has
         * been applied before this gate reads it.
         * @param active The mode in which input reaches the inner sink.
         */
        ModeGatedSink(
            ITickEventSink &inner,
            const AppModeState &mode,
            AppMode active) noexcept;

        ModeGatedSink(const ModeGatedSink &) = delete;
        ModeGatedSink(ModeGatedSink &&) = delete;

        ModeGatedSink &operator=(const ModeGatedSink &) = delete;
        ModeGatedSink &operator=(ModeGatedSink &&) = delete;

        /**
         * @brief Forward a tick event, or drop it.
         * @param event engine.tick is always forwarded; anything else
         * only when the app is in the gate's mode.
         */
        void handle(const TickEvent &event) override;

    private:
        ITickEventSink &inner;
        const AppModeState &mode;
        AppMode active;
    };

} // namespace antwika::game
