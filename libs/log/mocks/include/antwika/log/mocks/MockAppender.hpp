#pragma once

#include <gmock/gmock.h>

#include "antwika/log/Appender.hpp"

namespace antwika::log::mocks {

class MockAppender: public antwika::log::Appender {
public:
    MOCK_METHOD(void, append, (std::string_view message), (override));
};

} // namespace antwika::log::mocks
