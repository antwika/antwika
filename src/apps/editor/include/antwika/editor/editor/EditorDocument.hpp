#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <utility>

#include <antwika/map/MapFile.hpp>

#include "antwika/editor/editor/EditHistory.hpp"

namespace antwika::editor
{

    class EditorDocument final
    {
    public:
        antwika::map::Map map;

        [[nodiscard]] const std::string &getPath() const noexcept;

        [[nodiscard]] const std::string &getStartPath() const noexcept;

        [[nodiscard]] std::string getSiblingPath(
            const std::string &siblingName) const;

        [[nodiscard]] std::string getStartSiblingPath(
            const std::string &siblingName) const;

        void openAt(std::string openedPath);

        void startFrom(std::string openedPath);

        [[nodiscard]] bool isDirty() const noexcept;

        void markDirty() noexcept;

        void markSaved() noexcept;

        void push(antwika::editor::Snapshot stepSnapshot);

        void forgetHistory();

        [[nodiscard]] std::size_t getUndoCount() const noexcept;

        [[nodiscard]] std::optional<antwika::editor::Snapshot> undo(
            antwika::editor::Snapshot stoodSnapshot);

        [[nodiscard]] std::optional<antwika::editor::Snapshot> redo(
            antwika::editor::Snapshot stoodSnapshot);

    private:
        antwika::editor::EditHistory history;
        std::string mapPath;
        std::string startMapPath;
        bool dirty = false;
    };

}
