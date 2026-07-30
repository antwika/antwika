#include <memory>

#include <antwika/input/NullInputBackend.hpp>
#include <antwika/input/conformance/InputBackendConformance.hpp>

namespace antwika::input::conformance
{

    namespace
    {
        /**
         * @brief Builds a NullInputBackend for the shared conformance
         * suite.
         */
        struct NullInputBackendTraits
        {
            static std::unique_ptr<IInputBackend> create(ILogger &logger)
            {
                return std::make_unique<NullInputBackend>(logger);
            }
        };
    } // namespace

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Null,
        InputBackendConformance,
        NullInputBackendTraits);

} // namespace antwika::input::conformance
