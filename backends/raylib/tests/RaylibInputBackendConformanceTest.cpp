#include <memory>

#include <antwika/input/conformance/InputBackendConformanceTest.hpp>

#include "RaylibInputBackend.hpp"

namespace antwika::input::conformance
{

    namespace
    {
        struct RaylibInputBackendTraits final
        {
            static std::unique_ptr<IInputBackend> create(ILogger &logger)
            {
                return std::make_unique<raylib::RaylibInputBackend>(logger);
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Raylib,
        InputBackendConformanceTest,
        RaylibInputBackendTraits);

}
