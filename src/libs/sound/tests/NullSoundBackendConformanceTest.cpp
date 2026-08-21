#include <memory>

#include <antwika/sound/conformance/SoundBackendConformanceTest.hpp>

#include "antwika/sound/NullSoundBackend.hpp"

namespace antwika::sound::conformance
{

    namespace
    {
        struct NullSoundBackendTraits final
        {
            static std::unique_ptr<ISoundBackend> create(ILogger &logger)
            {
                return std::make_unique<NullSoundBackend>(logger);
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Null,
        SoundBackendConformanceTest,
        NullSoundBackendTraits);

}
