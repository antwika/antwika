#pragma once

#include <ostream>
#include <string_view>

#include "IAppender.hpp"

namespace antwika::log
{

    /**
     * @brief IAppender that writes messages to a std::ostream.
     */
    class StreamAppender : public IAppender
    {
    public:
        /**
         * @brief Construct the appender over an output stream.
         * @param stream The stream to write messages to. Must outlive this object.
         */
        explicit StreamAppender(std::ostream &stream);

        StreamAppender(const StreamAppender &) = delete;
        StreamAppender(StreamAppender &&) = delete;

        StreamAppender &operator=(const StreamAppender &) = delete;
        StreamAppender &operator=(StreamAppender &&) = delete;

        /**
         * @brief Write a message to the underlying stream.
         * @param message The message to write.
         */
        void append(std::string_view message) override;

    private:
        std::ostream &stream;
    };

} // namespace antwika::log
