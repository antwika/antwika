#pragma once

#include <cstdint>

#include <antwika/time/Tick.hpp>

#include "antwika/companion/Saying.hpp"

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

    /**
     * @brief How long one line stays in the speech bubble.
     *
     * Long enough to read at a glance and short enough that a companion
     * saying two things in a row is two bubbles rather than one.
     */
    inline constexpr Tick kSayingTicks = 3 * kTicksPerSecond;

    /**
     * @brief How often it finds something to say.
     *
     * Twice what a line lasts, so the bubble is empty for as long as it
     * is full and the chatter reads as occasional rather than constant.
     */
    inline constexpr Tick kChatterPeriodTicks = 6 * kTicksPerSecond;

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
        Tick sayingTicks = kSayingTicks;
        Tick chatterPeriodTicks = kChatterPeriodTicks;
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
     * @brief Everything one companion is, as a file has to remember it.
     *
     * Pet's own state made a value, and nothing else: the numbers a
     * session is resumed from, with no configuration in it, since which
     * numbers a companion is balanced with is this build's decision and
     * not a file's -- a saved one could quietly widen a day or hand a
     * companion twice the happiness it can hold.
     *
     * Deliberately not PetSnapshot, which is what a frame needs: a
     * snapshot has no counts, no bubble countdown and a hungry flag
     * rather than the hunger it was derived from, so resuming from one
     * would lose the session it claims to restore.
     *
     * Integers and enumerators throughout, no floating point, so a
     * companion is resumed at exactly the value it was suspended at on
     * every toolchain.
     */
    struct PetMemory
    {
        /** @brief How many ticks have been stepped. */
        Tick ticks = 0;

        /** @brief What it is doing. */
        PetState state = PetState::Awake;

        /** @brief What it is saying, or Saying::None for nothing. */
        Saying saying = Saying::None;

        /** @brief How much longer it goes on saying it. */
        Tick sayingTicksLeft = 0;

        std::uint32_t hunger = 0;
        std::uint32_t happiness = 0;
        std::uint32_t meals = 0;
        std::uint32_t disturbances = 0;
        std::uint32_t pesters = 0;

        /** @brief Whether tonight's rest has been interrupted. */
        bool disturbed = false;

        /**
         * @brief Compare two memories field by field.
         * @param other The memory to compare against.
         * @return Whether every field matches.
         */
        [[nodiscard]] bool operator==(const PetMemory &other) const
            = default;
    };

    /**
     * @brief A small animal with two needs, a day, a night and an end.
     *
     * Integer throughout, with no clock and no generator of its own, so
     * it is a pure function of how many times step() has been called and
     * when tap() was called between them. That is the whole reason a
     * recording of the taps replays to the same animal: nothing about it
     * reaches a *replay*, and everything about it is regenerated there.
     *
     * A companion does outlive the session it was in, through the
     * PetMemory remember() takes and the constructor that puts one back
     * -- but a replay neither reads nor writes one, for the reason
     * IPetStore and Companion.hpp both give: a replay that resumed from
     * whatever happened to be on the machine running it would reach a
     * different companion from the one it recorded.
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
     * What it is saying is decided here rather than by whatever draws
     * it, for the reason everything else here is: a renderer holding a
     * countdown, or picking a line from a generator of its own, is state
     * a replay cannot regenerate. Which idle line comes up is a hash of
     * the tick count -- not a draw from an antwika::rng generator, since
     * that would be a seed and a stream position for a save file to
     * carry and keep in step, where a hash of a number Pet already holds
     * is nothing at all.
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
         * @brief Construct a companion that has already lived.
         *
         * The way back in from a file, and the only one: every field a
         * session ends with is handed over at once rather than written
         * on afterwards through setters, so there is no moment at which
         * a half-restored companion exists.
         *
         * What it refuses is a memory no live companion could be in,
         * rather than repairing it -- a repaired companion is one
         * somebody never had, which is the rule apps/game's save reader
         * follows about its links. Nothing else is policed: a memory
         * whose state disagrees with the clock (Awake in the small
         * hours) is put right by the next step(), since falling asleep
         * and waking up are the clock and nothing else.
         *
         * @param config The numbers to balance it with.
         * @param memory What the last session left it as.
         * @throws CompanionError If the configuration is one no session
         * could be balanced on.
         * @throws SaveFormatError If the memory holds more hunger or
         * happiness than the configuration allows, or has perished with
         * happiness left -- or has none and has not. Two categories and
         * so two types: the first is a mistake in this build, the
         * second is a fact about somebody's file.
         */
        Pet(PetConfig config, const PetMemory &memory);

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
         * @brief Start again with a new companion.
         *
         * The one way out of Perished, which is otherwise a state
         * nothing leaves. It is not a resurrection: the animal that
         * perished is gone, and what this leaves behind is a companion
         * that has never been fed, never been woken and never said
         * anything -- its counts included, since a new companion
         * inheriting the last one's meals would be one session wearing
         * another's history.
         *
         * Legal at any time rather than only on a perished companion.
         * Which press means this is ReviveSink's decision and is made
         * against the button it hit-tests, and a rule enforced in two
         * places is one that can be enforced differently in each.
         */
        void revive();

        /**
         * @brief Take everything this companion is, as a value.
         *
         * The way out to a file, and remember()'s counterpart is the
         * constructor above rather than a setter: a round trip through
         * the two is the identity, which is what a test asserts.
         *
         * @return The memory.
         */
        [[nodiscard]] PetMemory remember() const;

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
         * @brief Get what it is saying, if anything.
         *
         * Which line comes up and when it comes up are both functions of
         * the tick count and of when tap() was called, so a replay says
         * the same words on the same ticks as the run it recorded.
         *
         * @return The line, or Saying::None while it is saying nothing.
         */
        [[nodiscard]] Saying saying() const noexcept;

        /**
         * @brief Get how much longer it will go on saying it.
         * @return The remaining ticks, or zero while it is silent.
         */
        [[nodiscard]] Tick sayingTicksLeft() const noexcept;

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

        void say(Saying line) noexcept;
        void speak() noexcept;

        PetConfig config;
        PetState petState = PetState::Awake;
        Saying said = Saying::None;
        Tick sayingLeft = 0;
        Tick elapsed = 0;
        std::uint32_t hungerLevel = 0;
        std::uint32_t happinessLevel = 0;
        std::uint32_t mealCount = 0;
        std::uint32_t disturbanceCount = 0;
        std::uint32_t pesterCount = 0;
        bool disturbedTonight = false;
    };

} // namespace antwika::companion
