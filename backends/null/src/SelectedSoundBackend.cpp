#include "antwika/sound/SelectedSoundBackend.hpp"

#include <memory>

#include "antwika/sound/NullSoundBackend.hpp"

namespace antwika::sound
{

    std::unique_ptr<ISoundBackend> makeSelectedSoundBackend(ILogger &logger)
    {
        return std::make_unique<NullSoundBackend>(logger);
    }

}
