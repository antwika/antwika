#include <memory>

#include <antwika/input/conformance/InputBackendConformance.hpp>

#include "RaylibInputBackend.hpp"

namespace antwika::input::conformance
{

    namespace
    {
        /**
         * @brief Builds a RaylibInputBackend for the shared suite.
         */
        struct RaylibInputBackendTraits
        {
            static std::unique_ptr<IInputBackend> create(ILogger &logger)
            {
                return std::make_unique<raylib::RaylibInputBackend>(logger);
            }
        };
    } // namespace

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Raylib,
        InputBackendConformance,
        RaylibInputBackendTraits);

} // namespace antwika::input::conformance
