#include "antwika/sound/SelectedSoundBackend.hpp"

#include <memory>

#include "RaylibSoundBackend.hpp"

namespace antwika::sound
{

    std::unique_ptr<ISoundBackend> createSelectedSoundBackend(ILogger &logger)
    {
        return std::make_unique<RaylibSoundBackend>(logger);
    }

}
