#include <antwika/voxel/VoxelMaterial.hpp>

namespace antwika::voxel
{

    bool occludes(const Kind neighbourKind, const Kind selfKind)
    {
        if (neighbourKind == Kind::Ramp)
        {
            return selfKind == Kind::Ramp;
        }

        return neighbourKind == Kind::Normal
               || neighbourKind == Kind::Ladder
               || selfKind == Kind::Water;
    }

}
