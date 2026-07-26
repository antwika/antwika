#pragma once

#include <gmock/gmock.h>

#include <antwika/log/IAppender.hpp>

using antwika::log::IAppender;

namespace antwika::log::mocks
{

    class MockAppender : public IAppender
    {
    public:
        MOCK_METHOD(void, append, (std::string_view message), (override));
    };

} // namespace antwika::log::mocks
