#include <memory>

#include <antwika/gfx/conformance/GfxBackendConformanceTest.hpp>

#include "antwika/gfx/BitmapBackend.hpp"

namespace antwika::gfx::conformance
{

    namespace
    {
        struct BitmapBackendTraits final
        {
            static std::unique_ptr<IGfxBackend> create(ILogger &logger)
            {
                return std::make_unique<BitmapBackend>(logger);
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Bitmap,
        GfxBackendConformanceTest,
        BitmapBackendTraits);

}
