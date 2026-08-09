#include <memory>

#include <antwika/input/conformance/InputBackendConformanceTest.hpp>

#include "Sdl3InputBackend.hpp"

namespace antwika::input::conformance
{

    namespace
    {
        struct Sdl3InputBackendTraits final
        {
            static std::unique_ptr<IInputBackend> create(ILogger &logger)
            {
                return std::make_unique<sdl3::Sdl3InputBackend>(logger);
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Sdl3,
        InputBackendConformanceTest,
        Sdl3InputBackendTraits);

}
