#include "antwika/log/StreamAppender.hpp"

namespace antwika::log
{

    StreamAppender::StreamAppender(std::ostream& stream): stream(stream) {
        
    }

    void StreamAppender::append(std::string_view message)
    {
        stream << message << "\n";
    }

} // namespace antwika::log
