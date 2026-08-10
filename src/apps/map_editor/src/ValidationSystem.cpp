#include "antwika/map_editor/ValidationSystem.hpp"

#include "antwika/map_editor/Commands.hpp"

namespace antwika::map_editor
{

    ValidationSystem::ValidationSystem(EditorStore &store)
        : store(store)
    {
    }

    void ValidationSystem::update(World &, antwika::time::Tick)
    {
        refreshReport(store.state);

        if (store.state.generateFailedTicks > 0)
        {
            --store.state.generateFailedTicks;
        }
    }

}
