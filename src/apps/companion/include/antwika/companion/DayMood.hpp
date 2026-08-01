#pragma once

#include <cstdint>

namespace antwika::companion
{

    /**
     * @brief What kind of day it is.
     *
     * A day picks one of these and keeps it, and each shortens exactly
     * one period, so a mood is something to plan against rather than a
     * surprise: the picture says which one is on before the first need
     * of the day has moved.
     *
     * Which mood a day has is a hash of the day's own number, in
     * `Pet`'s idle-chatter sense and for its reason -- a generator here
     * would be a seed and a stream position for a save file to carry and
     * keep in step, where a hash of a number the companion already holds
     * is nothing at all to carry.
     * A plain `day % count` would be the same four days in the same
     * order forever, which reads as a rota rather than as weather.
     */
    enum class DayMood : std::uint8_t
    {
        /** @brief Nothing in particular, and the commonest by half. */
        Ordinary = 0,

        /** @brief It gets hungry faster today. */
        Hungry,

        /** @brief It gets bored faster today. */
        Restless,

        /** @brief Everything it does costs more energy today. */
        Heavy,
    };

    /**
     * @brief Work out what kind of day a given day is.
     *
     * Half of all days are `Ordinary`, so a mood reads as a break from
     * the ordinary rather than as the ordinary state of things.
     *
     * @param day Which day it is, counted from the companion's first.
     * @return The mood.
     */
    [[nodiscard]] DayMood moodOn(std::uint32_t day);

} // namespace antwika::companion
