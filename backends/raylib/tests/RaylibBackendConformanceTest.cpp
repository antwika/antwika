#include <memory>

#include <antwika/gfx/conformance/GfxBackendConformance.hpp>

#include "RaylibBackend.hpp"

namespace antwika::gfx::conformance
{

    namespace
    {
        /**
         * @brief Builds a RaylibBackend for the shared conformance suite.
         */
        struct RaylibBackendTraits
        {
            static std::unique_ptr<IGfxBackend> create(ILogger &logger)
            {
                return std::make_unique<raylib::RaylibBackend>(logger);
            }
        };
    } // namespace

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Raylib,
        GfxBackendConformance,
        RaylibBackendTraits);

} // namespace antwika::gfx::conformance
