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

        if (map.exitTarget.empty())
        {
            if (playOnly)
            {
                savePlayerProgress();
            }

            running = false;

            return;
        }

        const auto was = mapPath;

        mapPath = (std::filesystem::path(mapPath).parent_path()
                   / map.exitTarget)
                      .string();

        if (!loadCurrentMap())
        {
            mapPath = was;

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
        return (std::filesystem::path(startMapPath).parent_path()
                / "progress.json")
            .string();
    }

    void Editor::savePlayerProgress()
    {
        map::saveProgress(
            game->progress(
                std::filesystem::path(mapPath)
                    .filename()
                    .string()),
            progressPath());
    }

}
