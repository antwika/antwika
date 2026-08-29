#pragma once

#include <antwika/geometry/GridCell.hpp>
#include <antwika/tilemap/Tilemap.hpp>

namespace antwika::editor
{

    class IEditSteps
    {
    public:
        IEditSteps() = default;

        IEditSteps(const IEditSteps &) = delete;
        IEditSteps(IEditSteps &&) = delete;

        IEditSteps &operator=(const IEditSteps &) = delete;
        IEditSteps &operator=(IEditSteps &&) = delete;

        virtual ~IEditSteps() = default;

        virtual void pushUndo() = 0;

        virtual void rebuildWorld()
        {
        }

        virtual void duplicateTile(geometry::GridCell, geometry::GridCell)
        {
        }

        virtual void wipeTile(tilemap::Tile)
        {
        }

        [[nodiscard]] virtual bool consumeAssignClick(tilemap::Tile)
        {
            return false;
        }

        [[nodiscard]] virtual bool blockedAsVariant()
        {
            return false;
        }
    };

}
