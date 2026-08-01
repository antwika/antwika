#pragma once

#include <cstdint>

#include <antwika/time/Tick.hpp>

#include "antwika/companion/DayMood.hpp"
#include "antwika/companion/LifeStage.hpp"
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

    /** @brief How often it gets one step hungrier while awake. */
    inline constexpr Tick kHungerPeriodTicks = 2 * kTicksPerSecond;

    /** @brief How often being famished costs it. */
    inline constexpr Tick kStarvePeriodTicks = 3 * kTicksPerSecond;

    /** @brief How often it loses one step of fun while awake. */
    inline constexpr Tick kFunDecayPeriodTicks = 2 * kTicksPerSecond;

    /** @brief How often having no fun left costs it. */
    inline constexpr Tick kFretPeriodTicks = 4 * kTicksPerSecond;

    /** @brief How often it wins one energy back while asleep. */
    inline constexpr Tick kRecoverPeriodTicks = kTicksPerSecond / 4;

    /** @brief How often a night gives one happiness back. */
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

    /** @brief The most fun it can be having. */
    inline constexpr std::uint32_t kFunMax = 10;

    /** @brief How much fun a new companion starts with. */
    inline constexpr std::uint32_t kFunStart = 10;

    /** @brief How much fun one game is worth. */
    inline constexpr std::uint32_t kPlayFun = 4;

    /** @brief How hungry one game leaves it. */
    inline constexpr std::uint32_t kPlayHunger = 2;

    /**
     * @brief How much energy one game costs.
     *
     * The whole tension of the day in one number: playing is what keeps
     * a companion happy, and happiness is what makes its energy last, so
     * the verb that keeps it alive is also the verb that spends its
     * life. A game is refused below this, and one played at exactly this
     * much leaves nothing at all -- which is a collapse, and a risk
     * somebody took rather than an accident.
     */
    inline constexpr std::uint32_t kPlayEnergy = 6;

    /** @brief How much happiness a game is worth. */
    inline constexpr std::uint32_t kPlayJoy = 1;

    /**
     * @brief How much energy the youngest companion may hold.
     *
     * The ceiling is not stored anywhere: it is this, plus what growing
     * up has added, less what collapsing has taken. So a companion's
     * remaining life is arithmetic over two numbers it already keeps --
     * the ticks it has lived and the times it has collapsed -- and there
     * is no third number a file could put out of step with them.
     */
    inline constexpr std::uint32_t kEnergyBase = 30;

    /** @brief How much more energy each stage of growing up allows. */
    inline constexpr std::uint32_t kStageEnergyBonus = 10;

    /**
     * @brief How much of the ceiling one collapse takes for good.
     *
     * Never given back, which is what makes a collapse the only thing in
     * the game with a permanent price, and what bounds a badly kept
     * companion's life at a handful of them.
     */
    inline constexpr std::uint32_t kCollapsePenalty = 10;

    /**
     * @brief How much of its ceiling it must have spent to go to bed.
     *
     * A percentage rather than a number of units, so it keeps its
     * meaning as growing up and collapsing move the ceiling around.
     * Bedtime is refused above this, which is the rule that stops sleep
     * being free: a companion has to spend the day near the edge before
     * it is allowed to recover from it.
     */
    inline constexpr std::uint32_t kTiredPercent = 40;

    /** @brief The happiest it can get. */
    inline constexpr std::uint32_t kHappinessMax = 10;

    /** @brief How happy it is when a session starts. */
    inline constexpr std::uint32_t kHappinessStart = 6;

    /** @brief From here up its energy lasts longest. */
    inline constexpr std::uint32_t kHappyBand = 8;

    /** @brief From here up its energy lasts well enough. */
    inline constexpr std::uint32_t kContentBand = 4;

    /** @brief How often a happy companion loses one energy. */
    inline constexpr Tick kDrainHappyTicks = 12;

    /** @brief How often a contented one does. */
    inline constexpr Tick kDrainContentTicks = 9;

    /** @brief How often an unhappy one does. */
    inline constexpr Tick kDrainLowTicks = 6;

    /** @brief How often a miserable one does. */
    inline constexpr Tick kDrainMiserableTicks = 4;

    /** @brief How much happiness being woken up costs. */
    inline constexpr std::uint32_t kDisturbCost = 2;

    /**
     * @brief How much happiness an unwanted attention costs.
     *
     * One price for all four refusals -- a meal it did not want, a game
     * it was too tired for, a bedtime it was too awake for, and a prod
     * at nothing in particular -- because they are the same mistake:
     * asking for something the companion is not in a state to give.
     */
    inline constexpr std::uint32_t kPesterCost = 1;

    /** @brief How long it stays an egg. */
    inline constexpr Tick kChildTicks = 40 * kTicksPerSecond;

    /** @brief How long until it is a teen. */
    inline constexpr Tick kTeenTicks = 100 * kTicksPerSecond;

    /** @brief How long until it is an adult. */
    inline constexpr Tick kAdultTicks = 200 * kTicksPerSecond;

    /** @brief How long until it is an elder. */
    inline constexpr Tick kElderTicks = 400 * kTicksPerSecond;

    /**
     * @brief What the companion is doing, which is all three of the
     * states it has.
     *
     * `Awake` and `Asleep` are decided by its energy rather than by a
     * clock: it goes to bed when it is put there or when it drops, and
     * it wakes when it has recovered or when somebody wakes it.
     * `Perished` is reached when its energy ceiling runs out and is
     * never left, which is what makes it an ordinary state rather than
     * the end of the run.
     */
    enum class PetState : std::uint8_t
    {
        /** @brief Up and about: it spends energy and wants things. */
        Awake = 0,

        /** @brief Recovering, and wanting to be left alone. */
        Asleep,

        /** @brief Its ceiling ran out, and nothing changes again. */
        Perished,
    };

    /**
     * @brief The numbers one companion is balanced with.
     *
     * A struct with defaults rather than constructor arguments, so a
     * test can shorten one period without restating the other thirty,
     * and so every default is visible next to what it means.
     */
    struct PetConfig
    {
        Tick hungerPeriodTicks = kHungerPeriodTicks;
        Tick starvePeriodTicks = kStarvePeriodTicks;
        Tick funDecayPeriodTicks = kFunDecayPeriodTicks;
        Tick fretPeriodTicks = kFretPeriodTicks;
        Tick recoverPeriodTicks = kRecoverPeriodTicks;
        Tick restPeriodTicks = kRestPeriodTicks;
        Tick sayingTicks = kSayingTicks;
        Tick chatterPeriodTicks = kChatterPeriodTicks;
        Tick drainHappyTicks = kDrainHappyTicks;
        Tick drainContentTicks = kDrainContentTicks;
        Tick drainLowTicks = kDrainLowTicks;
        Tick drainMiserableTicks = kDrainMiserableTicks;
        Tick childTicks = kChildTicks;
        Tick teenTicks = kTeenTicks;
        Tick adultTicks = kAdultTicks;
        Tick elderTicks = kElderTicks;
        std::uint32_t hungerMax = kHungerMax;
        std::uint32_t hungerThreshold = kHungerThreshold;
        std::uint32_t feedRelief = kFeedRelief;
        std::uint32_t feedJoy = kFeedJoy;
        std::uint32_t funMax = kFunMax;
        std::uint32_t funStart = kFunStart;
        std::uint32_t playFun = kPlayFun;
        std::uint32_t playHunger = kPlayHunger;
        std::uint32_t playEnergy = kPlayEnergy;
        std::uint32_t playJoy = kPlayJoy;
        std::uint32_t energyBase = kEnergyBase;
        std::uint32_t stageEnergyBonus = kStageEnergyBonus;
        std::uint32_t collapsePenalty = kCollapsePenalty;
        std::uint32_t tiredPercent = kTiredPercent;
        std::uint32_t happinessMax = kHappinessMax;
        std::uint32_t happinessStart = kHappinessStart;
        std::uint32_t happyBand = kHappyBand;
        std::uint32_t contentBand = kContentBand;
        std::uint32_t disturbCost = kDisturbCost;
        std::uint32_t pesterCost = kPesterCost;
    };

    /**
     * @brief Work out how grown up a companion of this age is.
     * @param config The numbers it is balanced with.
     * @param ticks How many ticks it has lived.
     * @return The stage.
     */
    [[nodiscard]] LifeStage stageAt(const PetConfig &config, Tick ticks);

    /**
     * @brief Work out how much energy a stage allows before collapses.
     * @param config The numbers it is balanced with.
     * @param stage How grown up it is.
     * @return The base ceiling.
     */
    [[nodiscard]] std::uint32_t baseEnergyFor(
        const PetConfig &config, LifeStage stage);

    /**
     * @brief Work out the ceiling a companion actually has.
     *
     * The base its stage allows, less what its collapses have taken,
     * floored at nothing -- and nothing is exactly the state this
     * application calls perished, which is why this is the one function
     * that says whether a companion is still alive.
     *
     * @param config The numbers it is balanced with.
     * @param ticks How many ticks it has lived.
     * @param collapses How many times it has dropped.
     * @return The ceiling, which is zero for a companion that is gone.
     */
    [[nodiscard]] std::uint32_t energyCeilingFor(
        const PetConfig &config, Tick ticks, std::uint32_t collapses);

    /**
     * @brief Everything one companion is, as a file has to remember it.
     *
     * Pet's own state made a value, and nothing else: the numbers a
     * session is resumed from, with no configuration in it, since which
     * numbers a companion is balanced with is this build's decision and
     * not a file's -- a saved one could quietly widen a stage or hand a
     * companion twice the energy it can hold.
     *
     * Neither the ceiling nor the stage is here, deliberately: both are
     * arithmetic over `ticks` and `collapses`, and a stored copy is a
     * second statement of one fact that a hand-edited file could put out
     * of step with the first.
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
        std::uint32_t fun = 0;
        std::uint32_t happiness = 0;
        std::uint32_t energy = 0;

        /** @brief Which day of its life it is on. */
        std::uint32_t day = 0;

        std::uint32_t meals = 0;
        std::uint32_t plays = 0;
        std::uint32_t disturbances = 0;
        std::uint32_t pesters = 0;

        /** @brief How many times its energy has run out entirely. */
        std::uint32_t collapses = 0;

        /** @brief Whether today began with somebody waking it. */
        bool woken = false;

        /**
         * @brief Compare two memories field by field.
         * @param other The memory to compare against.
         * @return Whether every field matches.
         */
        [[nodiscard]] bool operator==(const PetMemory &other) const
            = default;
    };

    /**
     * @brief A small animal whose energy is its life.
     *
     * Integer throughout, with no clock and no generator of its own, so
     * it is a pure function of how many times step() has been called and
     * of which verb was called between them. That is the whole reason a
     * recording of the presses replays to the same animal: nothing about
     * it reaches a *replay*, and everything about it is regenerated
     * there.
     *
     * A companion does outlive the session it was in, through the
     * PetMemory remember() takes and the constructor that puts one back
     * -- but a replay neither reads nor writes one, for the reason
     * IPetStore and Companion.hpp both give: a replay that resumed from
     * whatever happened to be on the machine running it would reach a
     * different companion from the one it recorded.
     *
     * **Energy is the life meter and happiness is the rate on it.**
     * Energy drains while it is awake, at a period its happiness picks
     * out of four bands, with a famished companion and a bored one each
     * paying a second drain on top. Energy is refilled by sleeping, and
     * sleeping is refused until it has spent enough of the day to be
     * tired -- so recovery has to be earned by risk rather than banked.
     * Energy reaching nothing is a *collapse* rather than an end: the
     * companion drops where it stands and loses a slice of its ceiling
     * for good. When that ceiling itself runs out it has perished, and
     * that is the only way one ever does. Starving, boredom and misery
     * do not kill; they only make a collapse arrive sooner, which is the
     * same rule this application always had -- every violation meets in
     * one place -- read out over a resource that visibly shrinks.
     *
     * Three verbs and one refusal are all that reach it. feed() while it
     * is hungry, play() while it has the energy to spare, putToBed()
     * while it is tired, and pester() for anything else somebody does at
     * it. **A press of any kind while it is asleep wakes it up**, which
     * costs happiness and, far worse, ends the night with the energy it
     * had rather than the energy it needed.
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
     * Which verb a press means is decided by *where* it landed, which is
     * PropSink's business and not this class's: PetLayout names one box
     * per prop, and the same boxes are what PetScene paints, so what
     * somebody sees and what they can hit cannot drift apart.
     */
    class Pet final
    {
    public:
        /**
         * @brief Construct a companion at the start of its first day.
         * @param config The numbers to balance it with.
         * @throws CompanionError If any period is zero, if it would
         * start with no happiness or no energy to lose, or if it could
         * be put to bed at full energy -- none is a balancing somebody
         * could have meant, and each divides, subtracts or compares
         * wrongly rather than merely playing badly.
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
         * follows about its links.
         *
         * @param config The numbers to balance it with.
         * @param memory What the last session left it as.
         * @throws CompanionError If the configuration is one no session
         * could be balanced on.
         * @throws SaveFormatError If the memory holds more hunger, fun,
         * happiness or energy than the configuration allows, or has
         * perished with a ceiling left -- or has none and has not. Two
         * categories and so two types: the first is a mistake in this
         * build, the second is a fact about somebody's file.
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
         * @brief Offer it a meal.
         *
         * A meal it wanted while it is awake and hungry, and an
         * unwanted attention otherwise.
         */
        void feed();

        /**
         * @brief Play with it.
         *
         * Fun and happiness for energy, which is the trade the whole day
         * is arranged around. Refused, and counted as an unwanted
         * attention, when there is not that much energy left to spend.
         */
        void play();

        /**
         * @brief Send it to bed.
         *
         * Accepted once it is tired enough, and an unwanted attention
         * while it is still wide awake -- which is the rule that keeps
         * a companion from simply sleeping its life safely away.
         */
        void putToBed();

        /**
         * @brief Bother it for no reason at all.
         *
         * What a press that landed on none of the props means. It is a
         * violation like any other refusal rather than nothing at all,
         * so sloppy aim has a price and the props are worth hitting.
         */
        void pester();

        /**
         * @brief Start again with a new companion.
         *
         * The one way out of Perished, which is otherwise a state
         * nothing leaves. It is not a resurrection: the animal that
         * perished is gone, and what this leaves behind is a companion
         * that has never been fed, never been played with and never said
         * anything -- its counts included, since a new companion
         * inheriting the last one's collapses would be born with a
         * ceiling it never spent.
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
         * @brief Take what it has been through, as a value.
         * @return The care record its form is decided from.
         */
        [[nodiscard]] CareRecord care() const;

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
         * @brief Get how much fun it is having, from zero to funMax.
         * @return The fun.
         */
        [[nodiscard]] std::uint32_t fun() const noexcept;

        /**
         * @brief Get how happy it is, from zero to happinessMax.
         * @return The happiness.
         */
        [[nodiscard]] std::uint32_t happiness() const noexcept;

        /**
         * @brief Get how much energy it has left.
         * @return The energy, which is what it lives on.
         */
        [[nodiscard]] std::uint32_t energy() const noexcept;

        /**
         * @brief Get how much energy it may hold.
         * @return The ceiling, which is zero once it has perished.
         */
        [[nodiscard]] std::uint32_t energyCeiling() const noexcept;

        /**
         * @brief Check whether it wants feeding.
         * @return True once hunger has reached the threshold.
         */
        [[nodiscard]] bool hungry() const noexcept;

        /**
         * @brief Check whether it has run out of fun.
         * @return True while there is none left.
         */
        [[nodiscard]] bool bored() const noexcept;

        /**
         * @brief Check whether it may be put to bed.
         * @return True once enough of its ceiling has been spent.
         */
        [[nodiscard]] bool tired() const noexcept;

        /**
         * @brief Check whether it is asleep.
         *
         * A state rather than a time of day: there is no clock here to
         * ask, which is exactly the point of coupling sleep to energy.
         *
         * @return True while it is recovering.
         */
        [[nodiscard]] bool night() const noexcept;

        /**
         * @brief Check whether somebody woke it to start today.
         * @return True when today began with a rude awakening.
         */
        [[nodiscard]] bool disturbed() const noexcept;

        /**
         * @brief Get how many ticks have been stepped.
         * @return The tick count, which the picture animates from.
         */
        [[nodiscard]] Tick ticks() const noexcept;

        /**
         * @brief Get which day of its life it is on.
         * @return The day, counted from nothing.
         */
        [[nodiscard]] std::uint32_t day() const noexcept;

        /**
         * @brief Get what kind of day today is.
         * @return The mood, which is a function of the day alone.
         */
        [[nodiscard]] DayMood mood() const noexcept;

        /**
         * @brief Get how grown up it is.
         * @return The stage, which is a function of the ticks alone.
         */
        [[nodiscard]] LifeStage stage() const noexcept;

        /**
         * @brief Get what it grew into.
         * @return The form, which is a function of the care alone.
         */
        [[nodiscard]] PetForm form() const;

        /**
         * @brief Get what it is saying, if anything.
         *
         * Which line comes up and when it comes up are both functions of
         * the tick count and of when a verb was called, so a replay says
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
         * @brief Get how many games it has played.
         * @return The count.
         */
        [[nodiscard]] std::uint32_t plays() const noexcept;

        /**
         * @brief Get how many times it has been woken up.
         * @return The count.
         */
        [[nodiscard]] std::uint32_t disturbances() const noexcept;

        /**
         * @brief Get how many things it was asked for and did not want.
         * @return The count.
         */
        [[nodiscard]] std::uint32_t pesters() const noexcept;

        /**
         * @brief Get how many times its energy has run out entirely.
         * @return The count, which is what bounds its whole life.
         */
        [[nodiscard]] std::uint32_t collapses() const noexcept;

        /**
         * @brief Get the numbers it was balanced with.
         * @return The configuration.
         */
        [[nodiscard]] const PetConfig &settings() const noexcept;

    private:
        [[nodiscard]] bool answered() noexcept;
        [[nodiscard]] Tick drainPeriod() const noexcept;
        [[nodiscard]] Tick hungerPeriod() const noexcept;
        [[nodiscard]] Tick funDecayPeriod() const noexcept;

        void gain(std::uint32_t amount) noexcept;
        void lose(std::uint32_t amount) noexcept;
        void spend(std::uint32_t amount) noexcept;
        void refuse(Saying line) noexcept;
        void collapse() noexcept;
        void perish() noexcept;
        void sleep() noexcept;
        void wake(bool rudely) noexcept;
        void recover() noexcept;
        void disturb() noexcept;

        void say(Saying line) noexcept;
        void speak() noexcept;

        PetConfig config;
        PetState petState = PetState::Awake;
        Saying said = Saying::None;
        Tick sayingLeft = 0;
        Tick elapsed = 0;
        std::uint32_t hungerLevel = 0;
        std::uint32_t funLevel = 0;
        std::uint32_t happinessLevel = 0;
        std::uint32_t energyLevel = 0;
        std::uint32_t dayNumber = 0;
        std::uint32_t mealCount = 0;
        std::uint32_t playCount = 0;
        std::uint32_t disturbanceCount = 0;
        std::uint32_t pesterCount = 0;
        std::uint32_t collapseCount = 0;
        bool wokenToday = false;
    };

} // namespace antwika::companion
