#pragma once

#include <chrono>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/time/ISleeper.hpp>

#include "antwika/tower_defence/BattleScene.hpp"
#include "antwika/tower_defence/Campaign.hpp"
#include "antwika/tower_defence/ScoreOverlay.hpp"

namespace antwika::tower_defence
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::IWindow;
    using antwika::gfx::Size;
    using antwika::time::ISleeper;

    /**
     * @brief Draws the level being fought and the score bar, once
     * per engine.tick.
     *
     * Rendering hangs off the tick loop without feeding back into it:
     * everything it reads arrives as an immutable BattleSnapshot and a
     * DrawList somebody else described, and nothing it does is visible
     * to any other sink.
     * Registered last, after CampaignSink and ScoreSink, so the frame
     * is of the state the tick ended with.
     *
     * It draws against the *configured* canvas rather than the size the
     * window reports, which is the same size TowerPlacementSink hit-
     * tests against.
     * Anything else would let what somebody sees and what they can click
     * disagree the moment a window manager had an opinion.
     *
     * It never closes the window and never asks it anything but whether
     * it is still open.
     */
    class RenderSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over everything it draws from.
         * @param window Window whose renderer receives each frame. Must
         * outlive this sink.
         * @param scene Draws the battle. Must outlive this sink.
         * @param campaign Snapshotted each tick. Must outlive this
         * sink.
         * @param overlay Holds the score bar's picture. Must outlive
         * this sink.
         * @param sleeper Paces the frames. Must outlive this sink.
         * @param framePeriod How long to hold each frame.
         * @param canvas The size everything is laid out against.
         */
        RenderSink(
            IWindow &window,
            const BattleScene &scene,
            const Campaign &campaign,
            const ScoreOverlay &overlay,
            ISleeper &sleeper,
            std::chrono::milliseconds framePeriod,
            Size canvas);

        RenderSink(const RenderSink &) = delete;
        RenderSink(RenderSink &&) = delete;

        RenderSink &operator=(const RenderSink &) = delete;
        RenderSink &operator=(RenderSink &&) = delete;

        /**
         * @brief Draw a frame if this is a tick.
         * @param event The event to fold in; anything but engine.tick is
         * ignored.
         */
        void handle(const TickEvent &event) override;

    private:
        IWindow &window;
        const BattleScene &scene;
        const Campaign &campaign;
        const ScoreOverlay &overlay;
        ISleeper &sleeper;
        std::chrono::milliseconds framePeriod;
        Size canvas;
    };

} // namespace antwika::tower_defence
