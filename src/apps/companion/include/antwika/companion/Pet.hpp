#pragma once

#include <cstdint>

#include <antwika/time/Tick.hpp>

#include "antwika/companion/DayMood.hpp"
#include "antwika/companion/LifeStage.hpp"
#include "antwika/companion/Saying.hpp"

namespace antwika::companion
{

    using antwika::time::Tick;

    inline constexpr Tick kTicksPerSecond = 20;

    inline constexpr Tick kHungerPeriodTicks = 2 * kTicksPerSecond;

    inline constexpr Tick kStarvePeriodTicks = 3 * kTicksPerSecond;

    inline constexpr Tick kFunDecayPeriodTicks = 2 * kTicksPerSecond;

    inline constexpr Tick kFretPeriodTicks = 4 * kTicksPerSecond;

    inline constexpr Tick kRecoverPeriodTicks = kTicksPerSecond / 4;

    inline constexpr Tick kRestPeriodTicks = 4 * kTicksPerSecond;

    inline constexpr Tick kSayingTicks = 3 * kTicksPerSecond;

    inline constexpr Tick kChatterPeriodTicks = 6 * kTicksPerSecond;

    inline constexpr std::uint32_t kHungerMax = 8;

    inline constexpr std::uint32_t kHungerThreshold = 4;

    inline constexpr std::uint32_t kFeedRelief = 4;

    inline constexpr std::uint32_t kFeedJoy = 1;

    inline constexpr std::uint32_t kFunMax = 10;

    inline constexpr std::uint32_t kFunStart = 10;

    inline constexpr std::uint32_t kPlayFun = 4;

    inline constexpr std::uint32_t kPlayHunger = 2;

    inline constexpr std::uint32_t kPlayEnergy = 6;

    inline constexpr std::uint32_t kPlayJoy = 1;

    inline constexpr std::uint32_t kEnergyBase = 30;

    inline constexpr std::uint32_t kStageEnergyBonus = 10;

    inline constexpr std::uint32_t kCollapsePenalty = 10;

    inline constexpr std::uint32_t kTiredPercent = 40;

    inline constexpr std::uint32_t kHappinessMax = 10;

    inline constexpr std::uint32_t kHappinessStart = 6;

    inline constexpr std::uint32_t kHappyBand = 8;

    inline constexpr std::uint32_t kContentBand = 4;

    inline constexpr Tick kDrainHappyTicks = 12;

    inline constexpr Tick kDrainContentTicks = 9;

    inline constexpr Tick kDrainLowTicks = 6;

    inline constexpr Tick kDrainMiserableTicks = 4;

    inline constexpr std::uint32_t kDisturbCost = 2;

    inline constexpr std::uint32_t kPesterCost = 1;

    inline constexpr Tick kChildTicks = 40 * kTicksPerSecond;

    inline constexpr Tick kTeenTicks = 100 * kTicksPerSecond;

    inline constexpr Tick kAdultTicks = 200 * kTicksPerSecond;

    inline constexpr Tick kElderTicks = 400 * kTicksPerSecond;

    enum class PetState : std::uint8_t
    {
        Awake = 0,

        Asleep,

        Perished,
    };

    [[nodiscard]] constexpr PetState enumBound(PetState) noexcept
    {
        return PetState::Perished;
    }

    struct PetConfig final
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

    [[nodiscard]] LifeStage stageAt(const PetConfig &config, Tick ticks);

    [[nodiscard]] std::uint32_t baseEnergyFor(
        const PetConfig &config, LifeStage stage);

    [[nodiscard]] std::uint32_t energyCeilingFor(
        const PetConfig &config, Tick ticks, std::uint32_t collapses);

    struct PetMemory final
    {
        Tick ticks = 0;

        PetState state = PetState::Awake;

        Saying saying = Saying::None;

        Tick sayingTicksLeft = 0;

        std::uint32_t hunger = 0;
        std::uint32_t fun = 0;
        std::uint32_t happiness = 0;
        std::uint32_t energy = 0;

        std::uint32_t day = 0;

        std::uint32_t meals = 0;
        std::uint32_t plays = 0;
        std::uint32_t disturbances = 0;
        std::uint32_t pesters = 0;

        std::uint32_t collapses = 0;

        bool woken = false;

        [[nodiscard]] bool operator==(const PetMemory &other) const
            = default;
    };

    class Pet final
    {
    public:
        explicit Pet(PetConfig config = {});

        Pet(PetConfig config, const PetMemory &memory);

        void step();

        void feed();

        void play();

        void putToBed();

        void pester();

        void revive();

        [[nodiscard]] PetMemory remember() const;

        [[nodiscard]] CareRecord care() const;

        [[nodiscard]] PetState state() const noexcept;

        [[nodiscard]] std::uint32_t hunger() const noexcept;

        [[nodiscard]] std::uint32_t fun() const noexcept;

        [[nodiscard]] std::uint32_t happiness() const noexcept;

        [[nodiscard]] std::uint32_t energy() const noexcept;

        [[nodiscard]] std::uint32_t energyCeiling() const noexcept;

        [[nodiscard]] bool hungry() const noexcept;

        [[nodiscard]] bool bored() const noexcept;

        [[nodiscard]] bool tired() const noexcept;

        [[nodiscard]] bool night() const noexcept;

        [[nodiscard]] bool disturbed() const noexcept;

        [[nodiscard]] Tick ticks() const noexcept;

        [[nodiscard]] std::uint32_t day() const noexcept;

        [[nodiscard]] DayMood mood() const noexcept;

        [[nodiscard]] LifeStage stage() const noexcept;

        [[nodiscard]] PetForm form() const;

        [[nodiscard]] Saying saying() const noexcept;

        [[nodiscard]] Tick sayingTicksLeft() const noexcept;

        [[nodiscard]] std::uint32_t meals() const noexcept;

        [[nodiscard]] std::uint32_t plays() const noexcept;

        [[nodiscard]] std::uint32_t disturbances() const noexcept;

        [[nodiscard]] std::uint32_t pesters() const noexcept;

        [[nodiscard]] std::uint32_t collapses() const noexcept;

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

}
