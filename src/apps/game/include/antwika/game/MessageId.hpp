#pragma once

#include <cstdint>

namespace antwika::game
{

    /**
     * @brief Every string the city builder shows, as symbolic ids.
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
         * @brief Toolbar: zoom one level in.
         */
        ToolbarZoomIn,

        /**
         * @brief Toolbar: zoom one level out.
         */
        ToolbarZoomOut,

        /**
         * @brief Toolbar: put the camera back where the run started.
         */
        ToolbarResetView,

        /**
         * @brief Toolbar: the current zoom level, with `{0}` as the level.
         */
        ToolbarZoomLevel,

        /**
         * @brief The palette's road tool.
         */
        ToolRoad,

        /**
         * @brief The palette's house tool.
         */
        ToolHouse,

        /**
         * @brief The palette's farm tool.
         */
        ToolFarm,

        /**
         * @brief The palette's clay pit tool.
         */
        ToolClayPit,

        /**
         * @brief The palette's workshop tool.
         */
        ToolWorkshop,

        /**
         * @brief The palette's storehouse tool.
         */
        ToolStorage,

        /**
         * @brief The palette's market tool.
         */
        ToolMarket,

        /**
         * @brief The palette's well tool.
         */
        ToolWell,

        /**
         * @brief The palette's doctor tool.
         */
        ToolDoctor,

        /**
         * @brief The palette's fire station tool.
         */
        ToolFireStation,

        /**
         * @brief The palette's engineer post tool.
         */
        ToolEngineerPost,

        /**
         * @brief Toolbar: hold the simulation still.
         */
        ToolbarPause,

        /**
         * @brief Toolbar: let the simulation go again.
         */
        ToolbarResume,

        /**
         * @brief Toolbar: open the menu modal over the city.
         */
        ToolbarMenu,

        /**
         * @brief Toolbar: the tick the run is on, `{0}` being it.
         */
        ToolbarTick,

        /**
         * @brief Main menu: the title over the card.
         */
        MenuTitle,

        /**
         * @brief Main menu: start a session.
         */
        MenuNewGame,

        /**
         * @brief Main menu: open the save and load screen.
         */
        MenuLoadGame,

        /**
         * @brief Main menu: look at the world.
         */
        MenuWorldMap,

        /**
         * @brief Main menu: end the run.
         */
        MenuQuit,

        /**
         * @brief Menu modal: the title over the card.
         */
        ModalTitle,

        /**
         * @brief Menu modal: leave the city for the main menu.
         */
        ModalMainMenu,

        /**
         * @brief Menu modal: put the modal away.
         */
        ModalResume,

        /**
         * @brief Save screen: the title over the card.
         */
        SaveTitle,

        /**
         * @brief Save screen: the picker with nothing in it.
         */
        SaveNoSaves,

        /**
         * @brief Save screen: the empty name field.
         */
        SaveNameNew,

        /**
         * @brief Save screen: write the session out.
         */
        SaveSave,

        /**
         * @brief Save screen: read the chosen session back.
         */
        SaveLoad,

        /**
         * @brief Save screen: go back where this was opened from.
         */
        SaveBack,

        /**
         * @brief Readout: a house.
         */
        BuildingHouse,

        /**
         * @brief Readout: a farm.
         */
        BuildingFarm,

        /**
         * @brief Readout: a clay pit.
         */
        BuildingClayPit,

        /**
         * @brief Readout: a workshop.
         */
        BuildingWorkshop,

        /**
         * @brief Readout: a storehouse.
         */
        BuildingStorage,

        /**
         * @brief Readout: a market.
         */
        BuildingMarket,

        /**
         * @brief Readout: a well.
         */
        BuildingWell,

        /**
         * @brief Readout: a doctor's surgery.
         */
        BuildingDoctor,

        /**
         * @brief Readout: a fire station.
         */
        BuildingFireStation,

        /**
         * @brief Readout: an engineer's post.
         */
        BuildingEngineerPost,

        /**
         * @brief Readout: a water carrier.
         */
        WalkerWaterCarrier,

        /**
         * @brief Readout: a doctor on their rounds.
         */
        WalkerDoctor,

        /**
         * @brief Readout: a fireman.
         */
        WalkerFireman,

        /**
         * @brief Readout: an engineer.
         */
        WalkerEngineer,

        /**
         * @brief Readout: a cart pusher.
         */
        WalkerCartPusher,

        /**
         * @brief Readout: a market buyer.
         */
        WalkerMarketBuyer,

        /**
         * @brief Readout: a market seller.
         */
        WalkerMarketSeller,

        /**
         * @brief Readout: somebody moving house.
         */
        WalkerMigrant,

        /**
         * @brief Readout: food.
         */
        ResourceFood,

        /**
         * @brief Readout: clay.
         */
        ResourceClay,

        /**
         * @brief Readout: pottery.
         */
        ResourcePottery,

        /**
         * @brief Readout: how much of a good is held, as `{0}` named, `{1}`
         *        held and `{2}` capacity.
         */
        ReadoutAmount,

        /**
         * @brief Readout: water reaching a building.
         */
        ServiceWater,

        /**
         * @brief Readout: health reaching a building.
         */
        ServiceHealth,

        /**
         * @brief Readout: fire safety reaching a building.
         */
        ServiceSafety,

        /**
         * @brief Readout: structural upkeep reaching a building.
         */
        ServiceStructure,

        /**
         * @brief Readout: how much of a service still reaches a building, as
         *        `{0}` named and `{1}` per cent left.
         */
        ReadoutCoverage,

        /**
         * @brief Readout: the bottom housing tier.
         */
        HousingTent,

        /**
         * @brief Readout: the second housing tier.
         */
        HousingShack,

        /**
         * @brief Readout: the third housing tier.
         */
        HousingHovel,

        /**
         * @brief Readout: the top housing tier.
         */
        HousingCottage,

        /**
         * @brief Readout: which tier a house is on, as `{0}` named.
         */
        ReadoutLevel,

        /**
         * @brief Toolbar: how many people live in the city, `{0}`.
         */
        ToolbarPopulation,

        /**
         * @brief Toolbar: what share of its jobs are staffed, `{0}` per cent.
         */
        ToolbarEmployment,

        /**
         * @brief The closed box of the toolbar's game menu.
         */
        ToolbarGameMenu,

        /**
         * @brief The overlay dropdown's view entry.
         */
        ViewMenu,

        /**
         * @brief The overlay dropdown's normal entry.
         */
        ViewNormal,

        /**
         * @brief The overlay dropdown's desirability entry.
         */
        ViewDesirability,

        /**
         * @brief The overlay dropdown's food entry.
         */
        ViewFood,

        /**
         * @brief The overlay dropdown's water entry.
         */
        ViewWater,

        /**
         * @brief The overlay dropdown's health entry.
         */
        ViewHealth,

        /**
         * @brief The overlay dropdown's fire entry.
         */
        ViewFire,

        /**
         * @brief The overlay dropdown's damage entry.
         */
        ViewDamage,

        /**
         * @brief The heading over the build palette.
         */
        ToolbarBuild,

        /**
         * @brief The menu item that empties the city.
         */
        MenuItemNewGame,

        /**
         * @brief The menu item that opens the save picker to write a session
         *        out.
         */
        MenuItemSaveGame,

        /**
         * @brief The menu item that opens the save picker to read a session
         *        back.
         */
        MenuItemLoadGame,

        /**
         * @brief The menu item that leaves the city for the main menu.
         */
        MenuItemMainMenu,

        /**
         * @brief The menu item that puts the city away for the world map.
         */
        MenuItemWorldMap,

        /**
         * @brief Readout: how full a house is, as `{0}` living there and `{1}`
         *        its tier's room.
         */
        ReadoutOccupancy,

        /**
         * @brief The main menu item that opens the key bindings screen.
         */
        MenuOptions,

        /**
         * @brief The heading over the key bindings screen.
         */
        OptionsTitle,

        /**
         * @brief One binding, as `{0}` the action and `{1}` the key.
         */
        OptionsRow,

        /**
         * @brief What a row shows while it is waiting to be told a key.
         */
        OptionsPress,

        /**
         * @brief How the key bindings screen is used.
         */
        OptionsHint,

        /**
         * @brief The key went to the action that asked for it.
         */
        OptionsBound,

        /**
         * @brief Another action already answers to that key.
         */
        OptionsTaken,

        /**
         * @brief The key is one the application spends above the tick loop.
         */
        OptionsReserved,

        /**
         * @brief Leave the key bindings screen.
         */
        OptionsBack,

        /**
         * @brief The action that holds the city still.
         */
        ActionPause,

        /**
         * @brief The action that takes the camera closer.
         */
        ActionZoomIn,

        /**
         * @brief The action that takes the camera further away.
         */
        ActionZoomOut,

        /**
         * @brief The action that puts the camera back.
         */
        ActionResetView,

        /**
         * @brief Toolbar: what is left in the bank, `{0}` being it.
         */
        ToolbarMoney,

        /**
         * @brief The palette's raze tool.
         */
        ToolRaze,

        /**
         * @brief Readout: a labourer carrying workforce to jobs.
         */
        WalkerLabourer,

        /**
         * @brief Readout: how many of a house's people lack a job, as
         *        `{0}` idle out of `{1}` living there.
         */
        ReadoutUnemployed,

        /**
         * @brief Readout: how staffed a workplace is, as `{0}` working
         *        out of `{1}` wanted.
         */
        ReadoutStaff,

        /**
         * @brief Readout: a building on fire.
         */
        RuinOnFire,

        /**
         * @brief Readout: the debris a fire leaves behind.
         */
        RuinDebris,

        /**
         * @brief Readout: the heading over the two risk lines.
         */
        ReadoutRiskTitle,

        /**
         * @brief Readout: how close to catching fire, as `{0}` per
         *        cent of the way there.
         */
        ReadoutFireRisk,

        /**
         * @brief Readout: how close to falling down, as `{0}` per
         *        cent of the way there.
         */
        ReadoutCollapseRisk,

        /**
         * @brief Readout: the heading over a house's stock lines.
         */
        ReadoutResourcesTitle,

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

} // namespace antwika::game
