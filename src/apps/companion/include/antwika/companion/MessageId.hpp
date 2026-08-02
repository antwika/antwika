#pragma once

#include <cstdint>

namespace antwika::companion
{

    /**
     * @brief Every string the companion shows, as symbolic ids.
     *
     * They live here rather than in antwika::i18n because a library
     * that enumerated its consumers' strings would be a library naming
     * its consumers.
     * What keeps that safe is that the list of every id there is, both
     * catalogues and the completeness check over them are in this
     * module too: see the MessageSet concept in
     * <antwika/i18n/MessageSet.hpp> and the suite MessagesTest.cpp
     * instantiates.
     *
     * A MessageId is never persisted, so its numbering is free and
     * adding, reordering or removing one needs no migration.
     */
    enum class MessageId : std::uint16_t
    {
        /**
         * @brief How hungry it is, `{0}` of its maximum.
         */
        Hunger,

        /**
         * @brief How happy it is, `{0}` of its maximum.
         */
        Happy,

        /**
         * @brief Awake and wanting nothing.
         */
        Awake,

        /**
         * @brief Awake and wanting a meal.
         */
        AwakeHungry,

        /**
         * @brief Asleep and undisturbed.
         */
        Asleep,

        /**
         * @brief Asleep and woken by a tap.
         */
        AsleepWoken,

        /**
         * @brief It has perished.
         */
        Gone,

        /**
         * @brief The button starting a new one.
         */
        NewPet,

        /**
         * @brief The label on the prop that feeds it.
         */
        PropFeed,

        /**
         * @brief The label on the prop that plays with it.
         */
        PropPlay,

        /**
         * @brief The label on the prop that puts it to bed.
         */
        PropSleep,

        /**
         * @brief Says: idle chatter.
         */
        SayHello,

        /**
         * @brief Says: idle chatter.
         */
        SayBored,

        /**
         * @brief Says: idle chatter.
         */
        SayNiceDay,

        /**
         * @brief Says: idle chatter.
         */
        SayLaLaLa,

        /**
         * @brief Says: it is hungry.
         */
        SayFeedMe,

        /**
         * @brief Says: it has been fed.
         */
        SayYumYum,

        /**
         * @brief Says: it was fed and did not want.
         */
        SayFull,

        /**
         * @brief Says: it was woken.
         */
        SayShhh,

        /**
         * @brief Says: it is sleeping quietly.
         */
        SayZzz,

        /**
         * @brief Says: it wants to be played with.
         */
        SayPlay,

        /**
         * @brief Says: it is being played with.
         */
        SayWheee,

        /**
         * @brief Says: it has no energy left to play.
         */
        SayTooTired,

        /**
         * @brief Says: it was sent to bed wide awake.
         */
        SayNotSleepy,

        /**
         * @brief Says: it has become tired enough for bed.
         */
        SayYawn,

        /**
         * @brief Says: a press landed on no prop.
         */
        SayPoked,

        /**
         * @brief Life stage: an egg.
         */
        StageEgg,

        /**
         * @brief Life stage: a child.
         */
        StageChild,

        /**
         * @brief Life stage: a teenager.
         */
        StageTeen,

        /**
         * @brief Life stage: an adult.
         */
        StageAdult,

        /**
         * @brief Life stage: an elder.
         */
        StageElder,

        /**
         * @brief Day mood: hunger comes on faster.
         */
        MoodHungry,

        /**
         * @brief Day mood: boredom comes on faster.
         */
        MoodRestless,

        /**
         * @brief Day mood: energy drains faster.
         */
        MoodHeavy,

        /**
         * @brief The day, `{0}`, its stage `{1}` and its mood `{2}`.
         */
        Day,

        /**
         * @brief The generation `{0}` and the best `{1}`.
         */
        Lineage,

        /**
         * @brief How many ids there are; not an id itself.
         *
         * Messages.cpp static_asserts its name table against this,
         * which is what makes an enumerator nobody listed a build
         * failure rather than a string that is silently in no
         * catalogue.
         */
        Count,
    };

} // namespace antwika::companion
