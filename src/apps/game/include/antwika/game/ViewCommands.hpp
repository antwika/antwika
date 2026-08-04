#pragma once

#include "antwika/game/Camera.hpp"
#include "antwika/game/PauseState.hpp"

namespace antwika::game
{

    /**
     * @brief The four things a player can ask of the view itself.
     *
     * Zoom in, zoom out, put the view back, and hold or release the
     * run -- the whole of what the bottom bar's first four buttons do
     * and what four of the bindable actions do.
     *
     * **It exists because there are two ways to ask for each of them.**
     * `UiSink` resolves a press on the bar and `HotkeySink` resolves a
     * bound key, and both used to spell the four answers out for
     * themselves -- which is why both took a `Camera &` to move *and* a
     * `Camera` to go back to, and why `UiSink`'s twelve-argument
     * constructor was handed the same camera twice, positionally, with
     * nothing but argument order telling the two apart.
     * A sink now dispatches to this and states nothing about what the
     * verbs mean, so the two routes to a zoom cannot drift and neither
     * sink names `home` at all.
     *
     * Nothing here is an event, on `GridSink`'s terms exactly: a
     * recording holds the press or the key, and a replay resolves it
     * against the same state and arrives at the same view.
     */
    class ViewCommands final
    {
    public:
        /**
         * @brief Construct the commands over what they move.
         * @param camera The view they pan and zoom; must outlive this
         * object.
         * @param pause The run they hold and release; must outlive
         * this object.
         * @param home Where resetView() puts the camera back to; a
         * copy, since it is a value the run was configured with rather
         * than a camera anybody moves.
         */
        ViewCommands(
            Camera &camera, PauseState &pause, Camera home) noexcept;

        ViewCommands(const ViewCommands &) = delete;
        ViewCommands(ViewCommands &&) = delete;

        ViewCommands &operator=(const ViewCommands &) = delete;
        ViewCommands &operator=(ViewCommands &&) = delete;

        /** @brief Take the view one step closer in. */
        void zoomIn() noexcept;

        /** @brief Take the view one step further out. */
        void zoomOut() noexcept;

        /** @brief Put the view back where the run started. */
        void resetView() noexcept;

        /**
         * @brief Hold the run if it is going, release it if it is
         * held.
         *
         * The opposite of what the bar was showing rather than a flip
         * of a flag: two players asking on one tick would otherwise
         * leave it running, and both asking for the same thing agree
         * -- see PauseState.
         */
        void togglePause() noexcept;

    private:
        Camera &camera;
        PauseState &pause;
        Camera home;
    };

} // namespace antwika::game
