#include <antwika/log/Level.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::takeExit()
    {
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

        document.openAt(document.getSiblingPath(document.map.exitTarget));

        if (!loadCurrentMap())
        {
            document.openAt(was);

            return;
        }

        keepMapForPlay();
        play.playing = true;
        aimPlayCamera();

        if (playOnly)
        {
            savePlayerProgress();
        }
    }

    std::string Editor::getProgressPath() const
    {
        return document.getStartSiblingPath("progress.json");
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
