#include <memory>

#include <antwika/gfx/conformance/GfxBackendConformance.hpp>

#include "Sdl3Backend.hpp"

namespace antwika::gfx::conformance
{

    namespace
    {
        /**
         * @brief Builds an Sdl3Backend for the shared conformance suite.
         */
        struct Sdl3BackendTraits
        {
            static std::unique_ptr<IGfxBackend> create(ILogger &logger)
            {
                return std::make_unique<sdl3::Sdl3Backend>(logger);
            }
        };
    } // namespace

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Sdl3,
        GfxBackendConformance,
        Sdl3BackendTraits);

} // namespace antwika::gfx::conformance
