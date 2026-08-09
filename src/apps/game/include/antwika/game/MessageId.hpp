#pragma once

#include <cstdint>

namespace antwika::game
{

    enum class MessageId : std::uint16_t
    {
        ToolbarZoomIn,

        ToolbarZoomOut,

        ToolbarResetView,

        ToolbarZoomLevel,

        ToolRoad,

        ToolHouse,

        ToolFarm,

        ToolClayPit,

        ToolWorkshop,

        ToolStorage,

        ToolMarket,

        ToolWell,

        ToolDoctor,

        ToolFireStation,

        ToolEngineerPost,

        ToolbarPause,

        ToolbarResume,

        ToolbarMenu,

        ToolbarTick,

        MenuTitle,

        MenuNewGame,

        MenuLoadGame,

        MenuWorldMap,

        MenuQuit,

        ModalTitle,

        ModalMainMenu,

        ModalResume,

        SaveTitle,

        SaveNoSaves,

        SaveNameNew,

        SaveSave,

        SaveLoad,

        SaveBack,

        BuildingHouse,

        BuildingFarm,

        BuildingClayPit,

        BuildingWorkshop,

        BuildingStorage,

        BuildingMarket,

        BuildingWell,

        BuildingDoctor,

        BuildingFireStation,

        BuildingEngineerPost,

        WalkerWaterCarrier,

        WalkerDoctor,

        WalkerFireman,

        WalkerEngineer,

        WalkerCartPusher,

        WalkerMarketBuyer,

        WalkerMarketSeller,

        WalkerMigrant,

        ResourceFood,

        ResourceClay,

        ResourcePottery,

        ReadoutAmount,

        ServiceWater,

        ServiceMedicine,

        HousingTent,

        HousingShack,

        HousingHovel,

        HousingCottage,

        ReadoutLevel,

        ToolbarPopulation,

        ToolbarEmployment,

        ToolbarGameMenu,

        ViewMenu,

        ViewNormal,

        ViewDesirability,

        ViewFood,

        ViewWater,

        ViewMedicine,

        ViewFire,

        ViewDamage,

        ToolbarBuild,

        MenuItemNewGame,

        MenuItemSaveGame,

        MenuItemLoadGame,

        MenuItemMainMenu,

        MenuItemWorldMap,

        ReadoutOccupancy,

        MenuOptions,

        OptionsTitle,

        OptionsRow,

        OptionsPress,

        OptionsHint,

        OptionsBound,

        OptionsTaken,

        OptionsReserved,

        OptionsLanguage,

        OptionsLanguageActive,

        OptionsBack,

        ActionPause,

        ActionZoomIn,

        ActionZoomOut,

        ActionResetView,

        OptionsKeyboard,

        KeyboardEnglish,

        KeyboardSwedish,

        ActionConsoleToggle,

        ActionConsoleExecute,

        ToolbarMoney,

        ToolRaze,

        WalkerLabourer,

        ReadoutUnemployed,

        ReadoutStaff,

        RuinOnFire,

        RuinDebris,

        ReadoutRiskTitle,

        ReadoutFireRisk,

        ReadoutCollapseRisk,

        ReadoutDiseaseRisk,

        ReadoutResourcesTitle,

        Count,
    };

}
