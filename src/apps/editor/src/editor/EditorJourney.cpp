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

        const auto was = document.path();

        document.openAt((std::filesystem::path(document.path()).parent_path()
                   / document.map.exitTarget)
                      .string());

        if (!loadCurrentMap())
        {
            document.openAt(was);

            return;
        }

        playing = true;
        activeView = map::View::World;
        aimPlayCamera();

        if (playOnly)
        {
            savePlayerProgress();
        }
    }

    std::string Editor::progressPath() const
    {
        return (std::filesystem::path(document.startPath()).parent_path()
                / "progress.json")
            .string();
    }

    void Editor::savePlayerProgress()
    {
        map::saveProgress(
            game->progress(
                std::filesystem::path(document.path())
                    .filename()
                    .string()),
            progressPath());
    }

}
