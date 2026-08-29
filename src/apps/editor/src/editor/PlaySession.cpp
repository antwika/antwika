#include "antwika/editor/editor/PlaySession.hpp"

namespace antwika::editor
{

    PlaySession::PlaySession(
        log::ILogger &logger,
        const map::Map &laidMap,
        const voxel::Voxels &solidVoxels)
        : world(logger),
          game(logger, world, laidMap, solidVoxels, patrolPositions)
    {
    }

}
