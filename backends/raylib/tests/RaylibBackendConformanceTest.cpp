#include <memory>

#include <antwika/gfx/conformance/GfxBackendConformanceTest.hpp>

#include "RaylibBackend.hpp"

namespace antwika::gfx::conformance
{

    namespace
    {
        struct RaylibBackendTraits final
        {
            static std::unique_ptr<IGfxBackend> create(ILogger &logger)
            {
                return std::make_unique<raylib::RaylibBackend>(logger);
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Raylib,
        GfxBackendConformanceTest,
        RaylibBackendTraits);

}
