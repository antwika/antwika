#pragma once

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/companion/Lineage.hpp"
#include "antwika/companion/Pet.hpp"
#include "antwika/companion/PetScene.hpp"

namespace antwika::companion
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::IWindow;
    using antwika::gfx::Size;

    /**
     * @brief Draws the companion, once per engine.tick.
     *
     * Rendering hangs off the tick loop without feeding back into it:
     * everything it reads arrives as an immutable PetSnapshot, and
     * nothing it does is visible to any other sink.
     * Registered after PropSink and PetSink, so the frame is of the
     * state the tick ended with.
     *
     * It draws against the *configured* canvas rather than the size the
     * window reports. Nothing here is hit-tested, so that costs nothing
     * and buys the same picture on every machine a recording is replayed
     * on -- which is the rule everywhere else in this project, kept here
     * so that reading one app does not teach the wrong thing.
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
         * @param scene Draws the companion. Must outlive this sink.
         * @param pet Snapshotted each tick. Must outlive this sink.
         * @param lineage The record behind it. Must outlive this sink.
         * @param canvas The size everything is laid out against.
         * @param consolePicture The debug console's sheet, painted
         * last so it stands over the companion. Must outlive this
         * sink.
         */
        RenderSink(
            IWindow &window,
            const PetScene &scene,
            const Pet &pet,
            const Lineage &lineage,
            Size canvas,
            const antwika::console::ConsolePicture &consolePicture);

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
        const PetScene &scene;
        const Pet &pet;
        const Lineage &lineage;
        Size canvas;
        const antwika::console::ConsolePicture &consolePicture;
    };

} // namespace antwika::companion
