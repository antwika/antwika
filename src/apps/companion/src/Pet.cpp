#include "antwika/companion/Pet.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "antwika/companion/CompanionError.hpp"
#include "antwika/companion/SaveFormatError.hpp"

namespace antwika::companion
{

    namespace
    {
        // What it says when nothing in particular is happening.
        // Four of them, so a session is not one line on repeat.
        constexpr std::array<Saying, 4> kIdleSayings{
            Saying::Hello,
            Saying::Bored,
            Saying::NiceDay,
            Saying::Silly};

        // How many stage bonuses each stage is worth, in its own order.
        // An elder is back to a child's ceiling rather than an adult's.
        // So old age takes room back, and can take the last of it.
        // A table rather than a switch, so this decides nothing at all.
        // Which leaves five stages and no branch between them.
        constexpr std::array<std::uint32_t, 5> kStageSteps{0, 1, 2, 3, 1};

        // Which idle line comes up is a hash of the tick it comes up on.
        // A generator would be a seed and a position for a save to carry.
        // One let out of step would say wrong things for a whole session.
        // A plain (tick / period) % count reads as a carousel instead.
        // It is the same four lines in the same order, forever.
        // A hash is neither: no state at all, and no order to notice.
        // It is a pure function of the one number Pet already is one of.
        // The arithmetic is the murmur3 finalizer over exact widths.
        // So which line comes up is the same on every toolchain.
        [[nodiscard]] std::size_t idleIndex(const Tick tick) noexcept
        {
            auto mixed = static_cast<std::uint64_t>(tick);
            mixed ^= mixed >> 33;
            mixed *= 0xff51afd7ed558ccdULL;
            mixed ^= mixed >> 33;

            return static_cast<std::size_t>(mixed % kIdleSayings.size());
        }

        // What a mood does to the one period it hurries along.
        // Two thirds of it, and never less than a single tick.
        // A period of nothing divides wrongly rather than playing hard.
        [[nodiscard]] Tick quickened(const Tick period) noexcept
        {
            const Tick shorter = period * 2 / 3;

            return shorter > 0 ? shorter : 1;
        }

        [[nodiscard]] std::uint32_t addCapped(
            const std::uint32_t value,
            const std::uint32_t amount,
            const std::uint32_t max) noexcept
        {
            const auto raised = value + amount;

            return raised > max ? max : raised;
        }

        // One check called many times rather than many checks.
        // A conjunction would be one branch per field to reach twice.
        // This is one branch, reached both ways by two tests.
        void requirePositive(const Tick value, const std::string_view what)
        {
            if (value != 0)
            {
                return;
            }

            throw CompanionError(
                "companion: " + std::string(what) + " must not be zero");
        }

        void requireAtMost(
            const std::uint32_t value,
            const std::uint32_t limit,
            const std::string_view what)
        {
            if (value <= limit)
            {
                return;
            }

            throw CompanionError(
                "companion: " + std::string(what)
                + " must not be more than the maximum it starts under");
        }

        PetConfig validated(const PetConfig config)
        {
            requirePositive(config.hungerPeriodTicks, "hungerPeriodTicks");
            requirePositive(config.starvePeriodTicks, "starvePeriodTicks");
            requirePositive(
                config.funDecayPeriodTicks, "funDecayPeriodTicks");
            requirePositive(config.fretPeriodTicks, "fretPeriodTicks");
            requirePositive(
                config.recoverPeriodTicks, "recoverPeriodTicks");
            requirePositive(config.restPeriodTicks, "restPeriodTicks");
            requirePositive(config.sayingTicks, "sayingTicks");
            requirePositive(
                config.chatterPeriodTicks, "chatterPeriodTicks");
            requirePositive(config.drainHappyTicks, "drainHappyTicks");
            requirePositive(config.drainContentTicks, "drainContentTicks");
            requirePositive(config.drainLowTicks, "drainLowTicks");
            requirePositive(
                config.drainMiserableTicks, "drainMiserableTicks");
            requirePositive(config.hungerMax, "hungerMax");
            requirePositive(config.funMax, "funMax");
            requirePositive(config.happinessMax, "happinessMax");
            requirePositive(config.happinessStart, "happinessStart");
            requirePositive(config.energyBase, "energyBase");
            requirePositive(config.playEnergy, "playEnergy");
            requirePositive(config.collapsePenalty, "collapsePenalty");
            requirePositive(config.tiredPercent, "tiredPercent");

            requireAtMost(
                config.happinessStart, config.happinessMax,
                "happinessStart");
            requireAtMost(config.funStart, config.funMax, "funStart");

            // A hundred per cent would let it be put to bed full.
            // Which is the one thing the tired gate exists to refuse.
            // Sleeping would then be free, and free recovery is no game.
            requireAtMost(config.tiredPercent, 99, "tiredPercent");

            return config;
        }

        // What a live companion could never be, refused not repaired.
        // A ceiling raised by a file is a gauge past its own end.
        // And perished is exactly "the ceiling ran out", both ways.
        // The ceiling is arithmetic over the ticks and the collapses.
        // So a file claiming one without the other is not this build's.
        void requireLivable(
            const PetConfig &config, const PetMemory &memory)
        {
            if (memory.hunger > config.hungerMax)
            {
                throw SaveFormatError(
                    "companion: a saved companion is hungrier than it "
                    "can be");
            }

            if (memory.fun > config.funMax)
            {
                throw SaveFormatError(
                    "companion: a saved companion is having more fun "
                    "than it can have");
            }

            if (memory.happiness > config.happinessMax)
            {
                throw SaveFormatError(
                    "companion: a saved companion is happier than it "
                    "can be");
            }

            const auto ceiling = energyCeilingFor(
                config, memory.ticks, memory.collapses);

            if (memory.energy > ceiling)
            {
                throw SaveFormatError(
                    "companion: a saved companion holds more energy "
                    "than its ceiling allows");
            }

            if ((ceiling == 0) != (memory.state == PetState::Perished))
            {
                throw SaveFormatError(
                    "companion: a saved companion has perished with a "
                    "ceiling left, or has none and lives on");
            }

            // A perished companion says nothing, and perish() sees to it.
            // It empties the bubble as the ceiling runs out.
            // step() then returns before another line could start.
            // So a grave mid-sentence is a state no live run reaches.
            // A file is not perish(), which is why this is stated here.
            // Refused rather than quietly emptied on the way in.
            // A repaired save is a session somebody never had.
            // The picture leans on the same rule as the state does.
            // PetScene's bubble and PetLayout's button share one box.
            // Each sits there because the other cannot be up at once.
            // So a bubble over a grave is a bubble over the button.
            if (memory.state == PetState::Perished
                && (memory.saying != Saying::None
                    || memory.sayingTicksLeft != 0))
            {
                throw SaveFormatError(
                    "companion: a saved companion has perished with "
                    "something still to say");
            }
        }
    } // namespace

    LifeStage stageAt(const PetConfig &config, const Tick ticks)
    {
        if (ticks < config.childTicks)
        {
            return LifeStage::Egg;
        }

        if (ticks < config.teenTicks)
        {
            return LifeStage::Child;
        }

        if (ticks < config.adultTicks)
        {
            return LifeStage::Teen;
        }

        if (ticks < config.elderTicks)
        {
            return LifeStage::Adult;
        }

        return LifeStage::Elder;
    }

    std::uint32_t baseEnergyFor(
        const PetConfig &config, const LifeStage stage)
    {
        const auto steps = kStageSteps[static_cast<std::size_t>(stage)];

        return config.energyBase + config.stageEnergyBonus * steps;
    }

    std::uint32_t energyCeilingFor(
        const PetConfig &config,
        const Tick ticks,
        const std::uint32_t collapses)
    {
        const auto base = baseEnergyFor(config, stageAt(config, ticks));
        const auto lost =
            static_cast<std::uint64_t>(config.collapsePenalty) * collapses;

        return lost < base ? base - static_cast<std::uint32_t>(lost) : 0;
    }

    Pet::Pet(const PetConfig config)
        : config(validated(config)),
          funLevel(this->config.funStart),
          happinessLevel(this->config.happinessStart),
          energyLevel(baseEnergyFor(this->config, LifeStage::Egg))
    {
    }

    // Delegating rather than a second initialiser list.
    // The configuration is checked in exactly one place that way.
    // And a field added to Pet cannot be forgotten by one of two.
    Pet::Pet(const PetConfig config, const PetMemory &memory)
        : Pet(config)
    {
        requireLivable(this->config, memory);

        petState = memory.state;
        said = memory.saying;
        sayingLeft = memory.sayingTicksLeft;
        elapsed = memory.ticks;
        hungerLevel = memory.hunger;
        funLevel = memory.fun;
        happinessLevel = memory.happiness;
        energyLevel = memory.energy;
        dayNumber = memory.day;
        mealCount = memory.meals;
        playCount = memory.plays;
        disturbanceCount = memory.disturbances;
        pesterCount = memory.pesters;
        collapseCount = memory.collapses;
        wokenToday = memory.woken;
    }

    void Pet::step()
    {
        // The clock runs whatever has happened to the companion.
        // So the picture keeps animating rather than freezing.
        // A grave does not move, but the day over it still does.
        ++elapsed;

        if (petState == PetState::Perished)
        {
            return;
        }

        // Growing old lowers the ceiling rather than raising it.
        // So a companion that has collapsed often can run out of room.
        // Living long enough is then an end of its own.
        // Which is why this is checked here and not only in collapse().
        if (energyCeiling() == 0)
        {
            perish();
            return;
        }

        // Before the needs below rather than after them.
        // A tick that runs the energy out has to end silent.
        // perish() clears the bubble, and a later line would sit on it.
        speak();

        if (petState == PetState::Asleep)
        {
            recover();
            return;
        }

        if (elapsed % hungerPeriod() == 0
            && hungerLevel < config.hungerMax)
        {
            ++hungerLevel;
        }

        if (elapsed % funDecayPeriod() == 0 && funLevel > 0)
        {
            --funLevel;
        }

        // Every drain of the tick is added up and spent once.
        // So there is one place a collapse can happen rather than three.
        // Three would each need the two after it guarded.
        std::uint32_t loss = 0;

        if (elapsed % drainPeriod() == 0)
        {
            ++loss;
        }

        // Famished and bored each cost happiness and energy on one beat.
        // Two rules with four effects, rather than four rules.
        // The happiness is the slow half: it widens the drain band.
        // The energy is the fast half, and is felt the same tick.
        if (hungerLevel >= config.hungerMax
            && elapsed % config.starvePeriodTicks == 0)
        {
            lose(1);
            ++loss;
        }

        if (bored() && elapsed % config.fretPeriodTicks == 0)
        {
            lose(1);
            ++loss;
        }

        const bool wasTired = tired();
        spend(loss);

        // The one telegraph a player gets that bedtime is now allowed.
        // Said only into an empty bubble, so it never cuts a line off.
        // And only while it is still standing.
        // One that collapsed this very tick has nothing to yawn about.
        if (!wasTired && tired() && petState == PetState::Awake
            && sayingLeft == 0)
        {
            say(Saying::Yawn);
        }
    }

    void Pet::feed()
    {
        if (answered())
        {
            return;
        }

        // Food a companion does not want is left uneaten.
        // Being offered it anyway is a violation like any other refusal.
        // It costs less than waking one does.
        // A bowl pushed at a full animal beats a night broken in half.
        if (!hungry())
        {
            refuse(Saying::NotHungry);
            return;
        }

        hungerLevel = hungerLevel > config.feedRelief
                          ? hungerLevel - config.feedRelief
                          : 0;
        ++mealCount;
        say(Saying::Yum);
        gain(config.feedJoy);
    }

    void Pet::play()
    {
        if (answered())
        {
            return;
        }

        // Refused below the price rather than clamped to what is left.
        // Half a game for half the energy would make play free.
        // And free exactly where it has to be expensive.
        if (energyLevel < config.playEnergy)
        {
            refuse(Saying::TooTired);
            return;
        }

        ++playCount;

        // Said and paid for before the energy goes, as a tap always was.
        // A game that spends the last of it may well be a collapse.
        // And a collapse that ends the ceiling clears the bubble itself.
        say(Saying::Wheee);
        gain(config.playJoy);
        funLevel = addCapped(funLevel, config.playFun, config.funMax);
        hungerLevel =
            addCapped(hungerLevel, config.playHunger, config.hungerMax);
        spend(config.playEnergy);
    }

    void Pet::putToBed()
    {
        if (answered())
        {
            return;
        }

        // The rule that stops sleep being free.
        // A companion has to have spent the day to be allowed the night.
        if (!tired())
        {
            refuse(Saying::NotSleepy);
            return;
        }

        sleep();
    }

    void Pet::pester()
    {
        if (answered())
        {
            return;
        }

        refuse(Saying::Poked);
    }

    void Pet::revive()
    {
        // What a new companion is, is what the constructor already says.
        // Written out again it would be a second list of one thing.
        // A field in one and not the other is the last one's collapses.
        // Nothing here holds a reference.
        // So assignment is the whole of it.
        *this = Pet(config);
    }

    PetMemory Pet::remember() const
    {
        return PetMemory{
            .ticks = elapsed,
            .state = petState,
            .saying = said,
            .sayingTicksLeft = sayingLeft,
            .hunger = hungerLevel,
            .fun = funLevel,
            .happiness = happinessLevel,
            .energy = energyLevel,
            .day = dayNumber,
            .meals = mealCount,
            .plays = playCount,
            .disturbances = disturbanceCount,
            .pesters = pesterCount,
            .collapses = collapseCount,
            .woken = wokenToday};
    }

    CareRecord Pet::care() const
    {
        return CareRecord{
            .meals = mealCount,
            .plays = playCount,
            .disturbances = disturbanceCount,
            .pesters = pesterCount,
            .collapses = collapseCount};
    }

    PetState Pet::state() const noexcept
    {
        return petState;
    }

    std::uint32_t Pet::hunger() const noexcept
    {
        return hungerLevel;
    }

    std::uint32_t Pet::fun() const noexcept
    {
        return funLevel;
    }

    std::uint32_t Pet::happiness() const noexcept
    {
        return happinessLevel;
    }

    std::uint32_t Pet::energy() const noexcept
    {
        return energyLevel;
    }

    std::uint32_t Pet::energyCeiling() const noexcept
    {
        return energyCeilingFor(config, elapsed, collapseCount);
    }

    bool Pet::hungry() const noexcept
    {
        return hungerLevel >= config.hungerThreshold;
    }

    bool Pet::bored() const noexcept
    {
        return funLevel == 0;
    }

    bool Pet::tired() const noexcept
    {
        return static_cast<std::uint64_t>(energyLevel) * 100
               <= static_cast<std::uint64_t>(energyCeiling())
                      * config.tiredPercent;
    }

    bool Pet::night() const noexcept
    {
        return petState == PetState::Asleep;
    }

    bool Pet::disturbed() const noexcept
    {
        return wokenToday;
    }

    Tick Pet::ticks() const noexcept
    {
        return elapsed;
    }

    std::uint32_t Pet::day() const noexcept
    {
        return dayNumber;
    }

    DayMood Pet::mood() const noexcept
    {
        return moodOn(dayNumber);
    }

    LifeStage Pet::stage() const noexcept
    {
        return stageAt(config, elapsed);
    }

    PetForm Pet::form() const
    {
        return formFor(care());
    }

    Saying Pet::saying() const noexcept
    {
        return said;
    }

    Tick Pet::sayingTicksLeft() const noexcept
    {
        return sayingLeft;
    }

    std::uint32_t Pet::meals() const noexcept
    {
        return mealCount;
    }

    std::uint32_t Pet::plays() const noexcept
    {
        return playCount;
    }

    std::uint32_t Pet::disturbances() const noexcept
    {
        return disturbanceCount;
    }

    std::uint32_t Pet::pesters() const noexcept
    {
        return pesterCount;
    }

    std::uint32_t Pet::collapses() const noexcept
    {
        return collapseCount;
    }

    const PetConfig &Pet::settings() const noexcept
    {
        return config;
    }

    // The one rule about a press that every verb shares.
    // A perished companion answers nothing at all.
    // A sleeping one answers everything the same way: it wakes up.
    // Stated once here rather than at the top of all four verbs.
    bool Pet::answered() noexcept
    {
        if (petState == PetState::Perished)
        {
            return true;
        }

        if (petState == PetState::Asleep)
        {
            disturb();
            return true;
        }

        return false;
    }

    Tick Pet::drainPeriod() const noexcept
    {
        Tick period = config.drainMiserableTicks;

        if (happinessLevel >= config.happyBand)
        {
            period = config.drainHappyTicks;
        }
        else if (happinessLevel >= config.contentBand)
        {
            period = config.drainContentTicks;
        }
        else if (happinessLevel > 0)
        {
            period = config.drainLowTicks;
        }

        return mood() == DayMood::Heavy ? quickened(period) : period;
    }

    Tick Pet::hungerPeriod() const noexcept
    {
        return mood() == DayMood::Hungry
                   ? quickened(config.hungerPeriodTicks)
                   : config.hungerPeriodTicks;
    }

    Tick Pet::funDecayPeriod() const noexcept
    {
        return mood() == DayMood::Restless
                   ? quickened(config.funDecayPeriodTicks)
                   : config.funDecayPeriodTicks;
    }

    void Pet::gain(const std::uint32_t amount) noexcept
    {
        happinessLevel =
            addCapped(happinessLevel, amount, config.happinessMax);
    }

    void Pet::lose(const std::uint32_t amount) noexcept
    {
        happinessLevel =
            happinessLevel > amount ? happinessLevel - amount : 0;
    }

    // Where every violation meets, as one place rather than three.
    // Neglect, boredom and a game too many all spend the same number.
    // Running it out is a collapse.
    void Pet::spend(const std::uint32_t amount) noexcept
    {
        if (amount == 0)
        {
            return;
        }

        energyLevel = energyLevel > amount ? energyLevel - amount : 0;

        if (energyLevel == 0)
        {
            collapse();
        }
    }

    void Pet::refuse(const Saying line) noexcept
    {
        ++pesterCount;
        say(line);
        lose(config.pesterCost);
    }

    // Dropping where it stands, which costs a slice of the ceiling.
    // Never given back, so a life is a handful of these and no more.
    void Pet::collapse() noexcept
    {
        ++collapseCount;

        // The ceiling is arithmetic over the count just raised.
        // So this reads the price the collapse has already cost.
        if (energyCeiling() == 0)
        {
            perish();
            return;
        }

        sleep();
    }

    void Pet::perish() noexcept
    {
        petState = PetState::Perished;
        energyLevel = 0;

        // Nothing about a perished companion changes again.
        // A bubble left over one would be the last thing it did.
        said = Saying::None;
        sayingLeft = 0;
    }

    void Pet::sleep() noexcept
    {
        petState = PetState::Asleep;
        wokenToday = false;
    }

    void Pet::wake(const bool rudely) noexcept
    {
        petState = PetState::Awake;
        wokenToday = rudely;
        ++dayNumber;
    }

    void Pet::recover() noexcept
    {
        if (elapsed % config.restPeriodTicks == 0)
        {
            gain(1);
        }

        if (elapsed % config.recoverPeriodTicks != 0)
        {
            return;
        }

        ++energyLevel;

        // A night ends when the energy is back rather than at an hour.
        // So a companion put to bed nearly empty sleeps a long night.
        // And one sent early wakes early, into a long day it must last.
        // Which is the whole of what bedtime is for.
        if (energyLevel >= energyCeiling())
        {
            energyLevel = energyCeiling();
            wake(false);
        }
    }

    // Waking it is the worst thing anybody can do to it.
    // The happiness is the small half of the cost.
    // The night ending with the energy it had is the large half.
    void Pet::disturb() noexcept
    {
        ++disturbanceCount;
        say(Saying::LetMeSleep);
        lose(config.disturbCost);
        wake(true);
    }

    void Pet::say(const Saying line) noexcept
    {
        said = line;
        sayingLeft = config.sayingTicks;
    }

    void Pet::speak() noexcept
    {
        // A line already up runs its course before another may start.
        // So a need cannot cut an answer off mid-word.
        // And the bubble never flickers between two consecutive ticks.
        if (sayingLeft > 0)
        {
            --sayingLeft;

            if (sayingLeft == 0)
            {
                said = Saying::None;
            }

            return;
        }

        if (elapsed % config.chatterPeriodTicks != 0)
        {
            return;
        }

        if (petState == PetState::Asleep)
        {
            say(Saying::Zzz);
            return;
        }

        // A need is worth saying and idle chatter is only worth having.
        // Hunger first, since it is the need with a bowl beside it.
        if (hungry())
        {
            say(Saying::FeedMe);
            return;
        }

        if (bored())
        {
            say(Saying::PlayWithMe);
            return;
        }

        say(kIdleSayings[idleIndex(elapsed)]);
    }

} // namespace antwika::companion
