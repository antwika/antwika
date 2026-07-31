#include "antwika/sound/SelectedSoundBackend.hpp"

#include <memory>

#include "Sdl3SoundBackend.hpp"

namespace antwika::sound
{

    std::unique_ptr<ISoundBackend> makeSelectedSoundBackend(ILogger &logger)
    {
        return std::make_unique<Sdl3SoundBackend>(logger);
    }

} // namespace antwika::sound
