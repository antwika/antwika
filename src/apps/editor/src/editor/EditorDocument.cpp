#include "antwika/editor/editor/EditorDocument.hpp"

#include <filesystem>

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

    std::string EditorDocument::getSiblingPath(
        const std::string &siblingName) const
    {
        return (std::filesystem::path(mapPath).parent_path() / siblingName)
            .string();
    }

    std::string EditorDocument::getStartSiblingPath(
        const std::string &siblingName) const
    {
        return (std::filesystem::path(startMapPath).parent_path()
                / siblingName)
            .string();
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

    void EditorDocument::push(antwika::editor::Snapshot stepSnapshot)
    {
        history.push(std::move(stepSnapshot));
        dirty = true;
    }

    void EditorDocument::forgetHistory()
    {
        history.clear();
    }

    std::size_t EditorDocument::getUndoCount() const noexcept
    {
        return history.getUndoCount();
    }

    std::optional<antwika::editor::Snapshot> EditorDocument::undo(
        antwika::editor::Snapshot stoodSnapshot)
    {
        return history.undo(std::move(stoodSnapshot));
    }

    std::optional<antwika::editor::Snapshot> EditorDocument::redo(
        antwika::editor::Snapshot stoodSnapshot)
    {
        return history.redo(std::move(stoodSnapshot));
    }

}
