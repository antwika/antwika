#pragma once

#include <string_view>

namespace antwika::log
{

    /**
     * @brief Destination that receives fully formatted log messages.
     */
    class IAppender
    {
    public:
        virtual ~IAppender() = default;

        /**
         * @brief Write a formatted message to the destination.
         * @param message The formatted message to write.
         */
        virtual void append(std::string_view message) = 0;
    };

} // namespace antwika::log
