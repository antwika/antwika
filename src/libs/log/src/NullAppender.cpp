#include "antwika/log/NullAppender.hpp"

namespace antwika::log
{

    void NullAppender::append(std::string_view)
    {
    }

} // namespace antwika::log
