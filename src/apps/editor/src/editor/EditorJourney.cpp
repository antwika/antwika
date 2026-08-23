#include <antwika/log/Level.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::takeExit()
    {
        if (!tryUnlockExit())
        {
            return;
        }

        logger.log(log::Level::Info, "the exit was reached");

        if (document.map.exitTarget.empty())
        {
            if (playOnly)
            {
                savePlayerProgress();
            }

            running = false;

            return;
        }

        const auto was = document.getPath();

        document.openAt((std::filesystem::path(document.getPath()).parent_path()
                   / document.map.exitTarget)
                      .string());

        if (!loadCurrentMap())
        {
            document.openAt(was);

            return;
        }

        play.playing = true;
        activeView = map::View::World;
        aimPlayCamera();

        if (playOnly)
        {
            savePlayerProgress();
        }
    }

    std::string Editor::getProgressPath() const
    {
        return (std::filesystem::path(document.getStartPath()).parent_path()
                / "progress.json")
            .string();
    }

    void Editor::savePlayerProgress()
    {
        map::saveProgress(
            play.game->getProgress(
                std::filesystem::path(document.getPath())
                    .filename()
                    .string()),
            getProgressPath());
    }

}
