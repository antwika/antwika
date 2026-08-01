#pragma once

#include <cstdint>

#include <antwika/time/Tick.hpp>

namespace antwika::companion
{

    using antwika::time::Tick;

    /**
     * @brief How many ticks one second of a companion's life is.
     *
     * Every other period below is written as a number of seconds times
     * this, rather than as a tick count somebody has to divide in their
     * head. Changing how fast the run is paced therefore changes one
     * constant, and every need keeps the same relationship to every
     * other one.
     *
     * Nothing here reads a clock: this is the exchange rate between a
     * tick and the wall clock main.cpp paces to, and the simulation only
     * ever counts ticks.
     */
    inline constexpr Tick kTicksPerSecond = 20;

    /** @brief How long the companion's day lasts. */
    inline constexpr Tick kDayTicks = 20 * kTicksPerSecond;

    /** @brief How long its night lasts. */
    inline constexpr Tick kNightTicks = 10 * kTicksPerSecond;

    /** @brief How often it gets one step hungrier while awake. */
    inline constexpr Tick kHungerPeriodTicks = 2 * kTicksPerSecond;

    /** @brief How often being famished costs it happiness. */
    inline constexpr Tick kStarvePeriodTicks = 3 * kTicksPerSecond;

    /** @brief How often an undisturbed night gives happiness back. */
    inline constexpr Tick kRestPeriodTicks = 4 * kTicksPerSecond;

    /** @brief The hungriest it can get. */
    inline constexpr std::uint32_t kHungerMax = 8;

    /** @brief From here up it wants feeding, and shows it. */
    inline constexpr std::uint32_t kHungerThreshold = 4;

    /** @brief How much hunger one meal takes away. */
    inline constexpr std::uint32_t kFeedRelief = 4;

    /** @brief How much happiness a well-timed meal is worth. */
    inline constexpr std::uint32_t kFeedJoy = 1;

    /** @brief How much happiness being woken up costs. */
    inline constexpr std::uint32_t kDisturbCost = 2;

    /**
     * @brief How much happiness a meal it did not want costs.
     *
     * Exactly kFeedJoy, so an unwanted meal is the equal and opposite of
     * a wanted one, and half of kDisturbCost, because pestering a waking
     * companion is a smaller sin than waking a sleeping one. From
     * kHappinessStart a stray tap is survivable and a night gives it
     * back, while tapping at it without pause runs it out -- which is
     * what makes tapping repeatedly a losing strategy rather than the
     * free action it used to be.
     */
    inline constexpr std::uint32_t kPesterCost = 1;

    /** @brief The happiest it can get. */
    inline constexpr std::uint32_t kHappinessMax = 10;

    /** @brief How happy it is when a session starts. */
    inline constexpr std::uint32_t kHappinessStart = 6;

    /**
     * @brief What the companion is doing, which is all three of the
     * states it has.
     *
     * Awake and Asleep follow the clock and nothing else; Perished is
     * reached from either and is never left, which is what makes it an
     * ordinary state rather than the end of the run.
     */
    enum class PetState : std::uint8_t
    {
        /** @brief Daytime: it wants feeding when it is hungry. */
        Awake = 0,

        /** @brief Night: it wants to be left alone. */
        Asleep,

        /** @brief Happiness reached zero, and nothing changes again. */
        Perished,
    };

    /**
     * @brief The numbers one companion is balanced with.
     *
     * A struct with defaults rather than constructor arguments, so a
     * test can shorten one period without restating the other eleven,
     * and so every default is visible next to what it means.
     */
    struct PetConfig
    {
        Tick dayTicks = kDayTicks;
        Tick nightTicks = kNightTicks;
        Tick hungerPeriodTicks = kHungerPeriodTicks;
        Tick starvePeriodTicks = kStarvePeriodTicks;
        Tick restPeriodTicks = kRestPeriodTicks;
        std::uint32_t hungerMax = kHungerMax;
        std::uint32_t hungerThreshold = kHungerThreshold;
        std::uint32_t feedRelief = kFeedRelief;
        std::uint32_t feedJoy = kFeedJoy;
        std::uint32_t disturbCost = kDisturbCost;
        std::uint32_t pesterCost = kPesterCost;
        std::uint32_t happinessMax = kHappinessMax;
        std::uint32_t happinessStart = kHappinessStart;
    };

    /**
     * @brief A small animal with two needs, a day, a night and an end.
     *
     * Integer throughout, with no clock and no generator of its own, so
     * it is a pure function of how many times step() has been called and
     * when tap() was called between them. That is the whole reason a
     * recording of the taps replays to the same animal: nothing about it
     * is ever persisted, and everything about it is regenerated.
     *
     * The two needs are deliberately opposite in what they ask for. Day
     * is when it wants feeding, and going unfed costs it happiness;
     * night is when it wants to be left alone, and a tap costs it
     * happiness and the rest of that night's recovery. A meal offered to
     * a companion that is not hungry is the third violation and the
     * gentlest: it wants attention rather than food, and being pestered
     * with a bowl it does not want costs it happiness too. Happiness
     * reaching zero is Perished, which nothing brings it back from --
     * see wiki/apps/companion.md for the numbers and why they are those.
     *
     * A tap is deliberately the only input, since the window is 256
     * pixels square and a pointer landing anywhere in it means the same
     * thing. There is therefore no layout to keep in step with the
     * scene, and no way for what somebody sees and what they can hit to
     * drift apart.
     */
    class Pet final
    {
    public:
        /**
         * @brief Construct a companion at the start of its first day.
         * @param config The numbers to balance it with.
         * @throws CompanionError If any period is zero, or if it would
         * start with no happiness to lose -- neither is a balancing
         * somebody could have meant, and both divide or subtract wrongly
         * rather than merely playing badly.
         */
        explicit Pet(PetConfig config = {});

        /**
         * @brief Advance one tick.
         *
         * The clock always moves, so the picture keeps animating over a
         * companion that has perished; nothing else about a perished one
         * ever changes again.
         */
        void step();

        /**
         * @brief Somebody tapped the window.
         *
         * What that means depends entirely on when it happens: a meal
         * while it is awake and hungry, an interruption while it is
         * asleep, and an annoyance while it is awake and full.
         *
         * Only a tap that lands on a companion which has already
         * perished means nothing at all, since nothing about one ever
         * changes again.
         */
        void tap();

        /**
         * @brief Get what it is doing.
         * @return The state.
         */
        [[nodiscard]] PetState state() const noexcept;

        /**
         * @brief Get how hungry it is, from zero to hungerMax.
         * @return The hunger.
         */
        [[nodiscard]] std::uint32_t hunger() const noexcept;

        /**
         * @brief Get how happy it is, from zero to happinessMax.
         * @return The happiness.
         */
        [[nodiscard]] std::uint32_t happiness() const noexcept;

        /**
         * @brief Check whether it wants feeding.
         * @return True once hunger has reached the threshold.
         */
        [[nodiscard]] bool hungry() const noexcept;

        /**
         * @brief Check whether the clock says it is night.
         *
         * A function of the tick count alone, so it is the same answer
         * on a live run and on a replay of it, and it stays true of a
         * perished companion -- the sun does not stop.
         *
         * @return True during the night half of the cycle.
         */
        [[nodiscard]] bool night() const noexcept;

        /**
         * @brief Check whether this night's rest has been interrupted.
         * @return True once a tap has landed since the night began.
         */
        [[nodiscard]] bool disturbed() const noexcept;

        /**
         * @brief Get how many ticks have been stepped.
         * @return The tick count, which the picture animates from.
         */
        [[nodiscard]] Tick ticks() const noexcept;

        /**
         * @brief Get how many meals it has eaten.
         * @return The count.
         */
        [[nodiscard]] std::uint32_t meals() const noexcept;

        /**
         * @brief Get how many times it has been woken up.
         * @return The count.
         */
        [[nodiscard]] std::uint32_t disturbances() const noexcept;

        /**
         * @brief Get how many meals it was offered and did not want.
         * @return The count.
         */
        [[nodiscard]] std::uint32_t pesters() const noexcept;

        /**
         * @brief Get the numbers it was balanced with.
         * @return The configuration.
         */
        [[nodiscard]] const PetConfig &settings() const noexcept;

    private:
        [[nodiscard]] Tick cycleTicks() const noexcept;

        void gain(std::uint32_t amount) noexcept;
        void lose(std::uint32_t amount) noexcept;

        PetConfig config;
        PetState petState = PetState::Awake;
        Tick elapsed = 0;
        std::uint32_t hungerLevel = 0;
        std::uint32_t happinessLevel = 0;
        std::uint32_t mealCount = 0;
        std::uint32_t disturbanceCount = 0;
        std::uint32_t pesterCount = 0;
        bool disturbedTonight = false;
    };

} // namespace antwika::companion
