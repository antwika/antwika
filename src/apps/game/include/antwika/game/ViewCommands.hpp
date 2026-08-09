#pragma once

#include "antwika/game/Camera.hpp"
#include "antwika/game/PauseState.hpp"

namespace antwika::game
{

    class ViewCommands final
    {
    public:
        ViewCommands(
            Camera &camera, PauseState &pause, Camera home) noexcept;

        ViewCommands(const ViewCommands &) = delete;
        ViewCommands(ViewCommands &&) = delete;

        ViewCommands &operator=(const ViewCommands &) = delete;
        ViewCommands &operator=(ViewCommands &&) = delete;

        void zoomIn() noexcept;

        void zoomOut() noexcept;

        void resetView() noexcept;

        void togglePause() noexcept;

    private:
        Camera &camera;
        PauseState &pause;
        Camera home;
    };

}
