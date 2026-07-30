#include <memory>

#include <antwika/input/conformance/InputBackendConformance.hpp>

#include "Sdl3InputBackend.hpp"

namespace antwika::input::conformance
{

    namespace
    {
        /**
         * @brief Builds an Sdl3InputBackend for the shared suite.
         */
        struct Sdl3InputBackendTraits
        {
            static std::unique_ptr<IInputBackend> create(ILogger &logger)
            {
                return std::make_unique<sdl3::Sdl3InputBackend>(logger);
            }
        };
    } // namespace

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Sdl3,
        InputBackendConformance,
        Sdl3InputBackendTraits);

} // namespace antwika::input::conformance
