#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <antwika/event/Event.hpp>

namespace antwika::console
{

    using antwika::event::Event;

    class ConsoleEvents final
    {
    public:
        /**
         * @brief Signs up a reader that every sent event must reach.
         *
         * @return The reader's own number, to take events with.
         */
        [[nodiscard]] std::size_t open();

        void send(Event event);

        /**
         * @brief Hands a reader the events it has not seen.
         *
         * @param reader A number an open() call gave out.
         * @return The events in the order they were sent.
         *
         * Requires: reader came from open() on this queue.
         * Ensures:  an event is dropped once every reader has taken it.
         */
        [[nodiscard]] std::vector<Event> take(std::size_t reader);

        [[nodiscard]] std::size_t pending() const noexcept;

        void refuse(const std::string &name, const std::string &reason);

        /**
         * @brief Hands over what the sinks refused to take.
         *
         * @return One line for each refused event, in the order they
         *         were refused.
         *
         * Ensures: a second call hands over nothing.
         */
        [[nodiscard]] std::vector<std::string> takeRefusals();

    private:
        void forget();

        std::vector<Event> queued;
        std::vector<std::size_t> taken;
        std::vector<std::string> refused;
    };

}
