#include <memory>

#include <antwika/gfx/conformance/GfxBackendConformanceTest.hpp>

#include "Sdl3Backend.hpp"

namespace antwika::gfx::conformance
{

    namespace
    {
        struct Sdl3BackendTraits final
        {
            static std::unique_ptr<IGfxBackend> create(ILogger &logger)
            {
                return std::make_unique<sdl3::Sdl3Backend>(logger);
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Sdl3,
        GfxBackendConformanceTest,
        Sdl3BackendTraits);

}
