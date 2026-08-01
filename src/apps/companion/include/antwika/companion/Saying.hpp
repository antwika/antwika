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
     * A new line is therefore appended at the end rather than filed
     * beside its relatives, which costs nothing in the save format --
     * that writes a name rather than a number, so appending here leaves
     * every file already on somebody's disk readable.
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

        /** @brief It is asleep. */
        Zzz,

        /** @brief It has run out of fun and wants playing with. */
        PlayWithMe,

        /** @brief It has just been played with. */
        Wheee,

        /** @brief It was asked to play with too little energy left. */
        TooTired,

        /** @brief It was sent to bed while it was still wide awake. */
        NotSleepy,

        /**
         * @brief It has just become tired enough to be put to bed.
         *
         * The one line that exists to be acted on rather than enjoyed.
         * Bedtime is refused above the tired threshold, so a player with
         * no way of knowing when that window opened would be playing
         * against a hidden rule rather than a hard one.
         */
        Yawn,

        /** @brief It was prodded somewhere that means nothing. */
        Poked,
    };

} // namespace antwika::companion
