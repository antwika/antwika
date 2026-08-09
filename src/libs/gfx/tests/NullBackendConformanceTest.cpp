#include <memory>

#include <antwika/gfx/conformance/GfxBackendConformanceTest.hpp>

#include "antwika/gfx/NullBackend.hpp"

namespace antwika::gfx::conformance
{

    namespace
    {
        struct NullBackendTraits final
        {
            static std::unique_ptr<IGfxBackend> create(ILogger &logger)
            {
                return std::make_unique<NullBackend>(logger);
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Null,
        GfxBackendConformanceTest,
        NullBackendTraits);

}
