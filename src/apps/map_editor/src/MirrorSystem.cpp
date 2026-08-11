#include "antwika/map_editor/MirrorSystem.hpp"

#include <cstddef>
#include <cstdint>

#include "antwika/map_editor/Components.hpp"
#include "antwika/map_editor/Widgets.hpp"

namespace antwika::map_editor
{

    MirrorSystem::MirrorSystem(EditorStore &store) : store(store)
    {
    }

    void MirrorSystem::update(World &world, antwika::time::Tick)
    {
        const auto &entities = store.state.map.entities();

        if (!fresh && entities == synced)
        {
            return;
        }

        for (const auto entity : spawned)
        {
            world.destroy(entity);
        }

        spawned.clear();

        for (std::size_t index = 0; index < entities.size(); ++index)
        {
            const auto cell = entityCellOf(entities[index]);
            const auto mirrored = world.create();

            world.add(
                mirrored,
                Marker{
                    .kind = markerKindOf(entities[index]),
                    .index = static_cast<std::uint32_t>(index)});
            world.add(
                mirrored,
                CellRef{
                    .column = cell.column,
                    .row = cell.row,
                    .level = entityLevelOf(entities[index])});
            spawned.push_back(mirrored);
        }

        synced = entities;
        fresh = false;

        if (store.ui.selected.has_value()
            && *store.ui.selected >= entities.size())
        {
            store.ui.selected.reset();
            loadEntityBuffers(store);
            return;
        }

        if (store.ui.selected.has_value()
            && !widgets::isField(store.ui.focus))
        {
            loadEntityBuffers(store);
        }
    }

}
