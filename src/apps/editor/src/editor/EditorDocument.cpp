#include "antwika/editor/editor/EditorDocument.hpp"

namespace antwika::editor
{

    const std::string &EditorDocument::getPath() const noexcept
    {
        return mapPath;
    }

    const std::string &EditorDocument::getStartPath() const noexcept
    {
        return startMapPath;
    }

    void EditorDocument::openAt(std::string openedPath)
    {
        mapPath = std::move(openedPath);
    }

    void EditorDocument::startFrom(std::string openedPath)
    {
        mapPath = std::move(openedPath);
        startMapPath = mapPath;
    }

    bool EditorDocument::isDirty() const noexcept
    {
        return dirty;
    }

    void EditorDocument::markDirty() noexcept
    {
        dirty = true;
    }

    void EditorDocument::markSaved() noexcept
    {
        dirty = false;
    }

    void EditorDocument::push(antwika::map::Snapshot stepSnapshot)
    {
        history.push(std::move(stepSnapshot));
        dirty = true;
    }

    void EditorDocument::forgetHistory()
    {
        history.clear();
    }

    std::optional<antwika::map::Snapshot> EditorDocument::undo(
        antwika::map::Snapshot stoodSnapshot)
    {
        return history.undo(std::move(stoodSnapshot));
    }

    std::optional<antwika::map::Snapshot> EditorDocument::redo(
        antwika::map::Snapshot stoodSnapshot)
    {
        return history.redo(std::move(stoodSnapshot));
    }

}
