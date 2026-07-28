#pragma once

#include <gmock/gmock.h>

#include <antwika/wfc/Domain.hpp>
#include <antwika/wfc/IConstraint.hpp>

namespace antwika::wfc::mocks
{

    using antwika::wfc::Domain;
    using antwika::wfc::IConstraint;

    /**
     * @brief GMock double for IConstraint.
     */
    class MockConstraint : public IConstraint
    {
    public:
        MOCK_METHOD(
            std::span<const std::size_t>, cells, (), (const, override));
        MOCK_METHOD(
            bool,
            prune,
            (std::vector<Domain> & wave),
            (const, override));
    };

} // namespace antwika::wfc::mocks
