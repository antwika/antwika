#include <memory>

#include <antwika/sound/conformance/SoundBackendConformanceTest.hpp>

#include "Sdl3SoundBackend.hpp"

namespace antwika::sound::conformance
{

    namespace
    {
        struct Sdl3SoundBackendTraits final
        {
            static std::unique_ptr<ISoundBackend> create(ILogger &logger)
            {
                return std::make_unique<Sdl3SoundBackend>(logger);
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Sdl3,
        SoundBackendConformanceTest,
        Sdl3SoundBackendTraits);

}
