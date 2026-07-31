#include <memory>

#include <antwika/sound/conformance/SoundBackendConformance.hpp>

#include "Sdl3SoundBackend.hpp"

namespace antwika::sound::conformance
{

    namespace
    {
        /**
         * @brief Builds an Sdl3SoundBackend for the shared suite.
         */
        struct Sdl3SoundBackendTraits
        {
            static std::unique_ptr<ISoundBackend> create(ILogger &logger)
            {
                return std::make_unique<Sdl3SoundBackend>(logger);
            }
        };
    } // namespace

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Sdl3,
        SoundBackendConformance,
        Sdl3SoundBackendTraits);

} // namespace antwika::sound::conformance
