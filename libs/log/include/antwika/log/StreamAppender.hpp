#pragma once

#include <ostream>
#include <string_view>

#include "IAppender.hpp"

namespace antwika::log
{

    class StreamAppender : public IAppender
    {
    public:
        explicit StreamAppender(std::ostream &stream);
        void append(std::string_view message) override;

    private:
        std::ostream &stream;
    };

} // namespace antwika::log
