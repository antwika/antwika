#include "antwika/game/ViewCommands.hpp"

namespace antwika::game
{

    ViewCommands::ViewCommands(
        Camera &camera, PauseState &pause, Camera home) noexcept
        : camera(camera), pause(pause), home(home)
    {
    }

    void ViewCommands::zoomIn() noexcept
    {
        camera.zoomIn();
    }

    void ViewCommands::zoomOut() noexcept
    {
        camera.zoomOut();
    }

    void ViewCommands::resetView() noexcept
    {
        camera = home;
    }

    void ViewCommands::togglePause() noexcept
    {
        pause.set(!pause.paused());
    }

}
