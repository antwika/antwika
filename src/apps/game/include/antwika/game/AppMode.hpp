#pragma once

#include <cstdint>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

namespace antwika::game
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    /**
     * @brief Which screen the application is on.
     *
     * A mode is a whole application state rather than a window drawn over
     * another one: in MainMenu the grid is not simulating behind the
     * menu, nothing is being placed and nothing is walking. There is no
     * modal stack here, and adding one would be the wrong shape -- a
     * menu is not "the grid, plus a dialog".
     *
     * New modes are added here as the app grows a world map and a
     * save/load screen; each one gets its own scene and its own sink, and
     * is reached by requesting it on the AppModeState below.
     */
    enum class AppMode : std::uint8_t
    {
        /**
         * @brief The screen a run starts on.
         */
        MainMenu = 0,

        /**
         * @brief The isometric grid: placing, walking, panning, zooming.
         */
        Playing,
    };

    /**
     * @brief Which mode the app is in, and which it is about to be in.
     *
     * **The mode is simulation state, not render state**, for exactly the
     * reason the camera is (see Camera.hpp and blog/013): which mode you
     * are in decides what a click *means*, so a renderer-owned mode would
     * leave a replay resolving recorded clicks against a different
     * screen. It therefore lives here, is folded by the tick path, and is
     * regenerated from the recorded input rather than persisted -- no
     * event of any kind is defined for changing it.
     *
     * A change is *staged* and applied at the tick boundary, the way
     * ecs::World double-buffers its components, and for the same reason
     * an app would otherwise get wrong: the sinks of one tick run in a
     * fixed order over each event, so a mode changed part-way through an
     * event would let one click be read by two modes -- pressing "New
     * Game" would both leave the menu and lay a path tile on the grid it
     * revealed. Staging makes that not expressible.
     *
     * It is an ITickEventSink so that the boundary is a place rather than
     * a convention: **register it immediately after InputFold**, ahead of
     * every sink that is gated on a mode, so that the tick which applies
     * a change applies it before anything reads it.
     */
    class AppModeState final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the state over the mode a run starts in.
         * @param initial The mode both current and staged begin as. The
         * application leaves this defaulted; a test whose subject is the
         * grid starts in Playing rather than clicking its way there, the
         * same way it sets GameConfig::maxTicks rather than dispatching a
         * stop.
         */
        explicit AppModeState(AppMode initial = AppMode::MainMenu) noexcept
            : current(initial), staged(initial)
        {
        }

        AppModeState(const AppModeState &) = delete;
        AppModeState(AppModeState &&) = delete;

        AppModeState &operator=(const AppModeState &) = delete;
        AppModeState &operator=(AppModeState &&) = delete;

        /**
         * @brief Get the mode this tick is being run in.
         * @return The committed mode.
         */
        [[nodiscard]] AppMode mode() const noexcept
        {
            return current;
        }

        /**
         * @brief Get the mode the next tick will be run in.
         * @return The staged mode, equal to mode() unless something has
         * asked for a change during this tick.
         */
        [[nodiscard]] AppMode next() const noexcept
        {
            return staged;
        }

        /**
         * @brief Ask to be in another mode from the next tick on.
         * @param mode The mode to change to.
         */
        void request(AppMode mode) noexcept
        {
            staged = mode;
        }

        /**
         * @brief Apply a staged change at the tick boundary.
         * @param event engine.tick commits whatever was staged; anything
         * else is ignored, since only the boundary may change the mode.
         */
        void handle(const TickEvent &event) override;

    private:
        AppMode current;
        AppMode staged;
    };

} // namespace antwika::game
