#include <memory>

#include <antwika/gfx/conformance/GfxBackendConformance.hpp>

#include "antwika/gfx/NullBackend.hpp"

namespace antwika::gfx::conformance
{

    namespace
    {
        /**
         * @brief Builds a NullBackend for the shared conformance suite.
         */
        struct NullBackendTraits
        {
            static std::unique_ptr<IGfxBackend> create(ILogger &logger)
            {
                return std::make_unique<NullBackend>(logger);
            }
        };
    } // namespace

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Null,
        GfxBackendConformance,
        NullBackendTraits);

} // namespace antwika::gfx::conformance
