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
        constexpr std::array<Saying, 4> kIdleSayings{
            Saying::Hello,
            Saying::Bored,
            Saying::NiceDay,
            Saying::Silly};

        constexpr std::array<std::uint32_t, 5> kStageSteps{0, 1, 2, 3, 1};

        [[nodiscard]] std::size_t idleIndex(const Tick tick) noexcept
        {
            auto mixed = static_cast<std::uint64_t>(tick);
            mixed ^= mixed >> 33;
            mixed *= 0xff51afd7ed558ccdULL;
            mixed ^= mixed >> 33;

            return static_cast<std::size_t>(mixed % kIdleSayings.size());
        }

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

            requireAtMost(config.tiredPercent, 99, "tiredPercent");

            return config;
        }

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

            if (memory.state == PetState::Perished
                && (memory.saying != Saying::None
                    || memory.sayingTicksLeft != 0))
            {
                throw SaveFormatError(
                    "companion: a saved companion has perished with "
                    "something still to say");
            }
        }
    }

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
        ++elapsed;

        if (petState == PetState::Perished)
        {
            return;
        }

        if (energyCeiling() == 0)
        {
            perish();
            return;
        }

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

        std::uint32_t loss = 0;

        if (elapsed % drainPeriod() == 0)
        {
            ++loss;
        }

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

        if (energyLevel < config.playEnergy)
        {
            refuse(Saying::TooTired);
            return;
        }

        ++playCount;

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

    void Pet::collapse() noexcept
    {
        ++collapseCount;

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

        if (energyLevel >= energyCeiling())
        {
            energyLevel = energyCeiling();
            wake(false);
        }
    }

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

}
