#include <memory>

#include <antwika/sound/conformance/SoundBackendConformance.hpp>

#include "antwika/sound/NullSoundBackend.hpp"

namespace antwika::sound::conformance
{

    namespace
    {
        /**
         * @brief Builds a NullSoundBackend for the shared suite.
         */
        struct NullSoundBackendTraits
        {
            static std::unique_ptr<ISoundBackend> create(ILogger &logger)
            {
                return std::make_unique<NullSoundBackend>(logger);
            }
        };
    } // namespace

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Null,
        SoundBackendConformance,
        NullSoundBackendTraits);

} // namespace antwika::sound::conformance
