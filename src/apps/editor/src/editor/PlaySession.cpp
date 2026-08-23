#include "antwika/editor/editor/PlaySession.hpp"

namespace antwika::editor
{

    PlaySession::PlaySession(
        log::ILogger &logger, const voxel::Voxels &solidVoxels)
        : world(logger),
          game(logger, world, solidVoxels, patrolPositions)
    {
    }

}
