#include <memory>

#include <antwika/sound/conformance/SoundBackendConformanceTest.hpp>

#include "RaylibSoundBackend.hpp"

namespace antwika::sound::conformance
{

    namespace
    {
        struct RaylibSoundBackendTraits final
        {
            static std::unique_ptr<ISoundBackend> create(ILogger &logger)
            {
                return std::make_unique<RaylibSoundBackend>(logger);
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Raylib,
        SoundBackendConformanceTest,
        RaylibSoundBackendTraits);

}
