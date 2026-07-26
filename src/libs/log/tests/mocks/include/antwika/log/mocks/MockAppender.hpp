#pragma once

#include <gmock/gmock.h>

#include <antwika/log/IAppender.hpp>

namespace antwika::log::mocks
{

    using antwika::log::IAppender;

    class MockAppender : public IAppender
    {
    public:
        MOCK_METHOD(void, append, (std::string_view message), (override));
    };

} // namespace antwika::log::mocks
