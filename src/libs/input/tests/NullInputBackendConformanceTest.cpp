#include <memory>

#include <antwika/input/NullInputBackend.hpp>
#include <antwika/input/conformance/InputBackendConformanceTest.hpp>

namespace antwika::input::conformance
{

    namespace
    {
        struct NullInputBackendTraits final
        {
            static std::unique_ptr<IInputBackend> create(ILogger &logger)
            {
                return std::make_unique<NullInputBackend>(logger);
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Null,
        InputBackendConformanceTest,
        NullInputBackendTraits);

}
