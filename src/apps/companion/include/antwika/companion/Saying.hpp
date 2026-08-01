#pragma once

#include <cstdint>

namespace antwika::companion
{

    /**
     * @brief One line a companion may have in its speech bubble.
     *
     * The enumerator is what the simulation decides and what the
     * snapshot carries; the words themselves belong to whatever draws
     * them, exactly as a building's kind and its caption are apart in
     * apps/game.
     * So `Pet` never holds a sentence, and changing what a line says is
     * not a change to anything a replay reproduces.
     *
     * The order matters in one place only: the scene indexes its table
     * of words by the enumerator, so a line added in the middle of this
     * list has to be added in the middle of that one, which a
     * static_assert on the table's size is there to catch.
     */
    enum class Saying : std::uint8_t
    {
        /** @brief Nothing is being said, and no bubble is drawn. */
        None = 0,

        /** @brief Idle chatter. */
        Hello,

        /** @brief Idle chatter. */
        Bored,

        /** @brief Idle chatter. */
        NiceDay,

        /** @brief Idle chatter. */
        Silly,

        /** @brief It is hungry and would like that seen to. */
        FeedMe,

        /** @brief It has just been fed. */
        Yum,

        /** @brief It has just been offered a meal it did not want. */
        NotHungry,

        /** @brief It has just been woken up. */
        LetMeSleep,

        /** @brief It is asleep and undisturbed. */
        Zzz,
    };

} // namespace antwika::companion
