#pragma once

#include <string_view>

#include "antwika/log/IAppender.hpp"

namespace antwika::log
{

    class NullAppender : public IAppender
    {
    public:
        void append(std::string_view message) override;
    };

} // namespace antwika::log
