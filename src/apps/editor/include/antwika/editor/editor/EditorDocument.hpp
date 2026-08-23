#pragma once

#include <optional>
#include <string>
#include <utility>

#include <antwika/map/EditHistory.hpp>
#include <antwika/map/MapFile.hpp>

namespace antwika::editor
{

    class EditorDocument final
    {
    public:
        antwika::map::Map map;

        [[nodiscard]] const std::string &getPath() const noexcept;

        [[nodiscard]] const std::string &getStartPath() const noexcept;

        void openAt(std::string openedPath);

        void startFrom(std::string openedPath);

        [[nodiscard]] bool isDirty() const noexcept;

        void markDirty() noexcept;

        void markSaved() noexcept;

        void push(antwika::map::Snapshot stepSnapshot);

        void forgetHistory();

        [[nodiscard]] std::optional<antwika::map::Snapshot> undo(
            antwika::map::Snapshot stoodSnapshot);

        [[nodiscard]] std::optional<antwika::map::Snapshot> redo(
            antwika::map::Snapshot stoodSnapshot);

    private:
        antwika::map::EditHistory history;
        std::string mapPath;
        std::string startMapPath;
        bool dirty = false;
    };

}
