#pragma once

#include <string_view>

#include "IAppender.hpp"

namespace antwika::log
{

    class NullAppender final : public IAppender
    {
    public:
        void append(std::string_view message) override;
    };

}
