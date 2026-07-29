#pragma once

#include <string_view>

#include "IAppender.hpp"

namespace antwika::log
{

    /**
     * @brief IAppender that discards every message it receives.
     */
    class NullAppender final : public IAppender
    {
    public:
        /**
         * @brief Discard the given message.
         * @param message The message to discard.
         */
        void append(std::string_view message) override;
    };

} // namespace antwika::log
