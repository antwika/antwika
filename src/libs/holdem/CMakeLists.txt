add_library(antwika_holdem
    src/ActionType.cpp
    src/BettingRound.cpp
    src/CardText.cpp
    src/Deck.cpp
    src/HandCategory.cpp
    src/HandEvaluator.cpp
    src/HandText.cpp
    src/Pots.cpp
    src/SplitMix64Rng.cpp
    src/Stage.cpp
    src/Table.cpp
    src/TableRunner.cpp
)

add_library(antwika::holdem ALIAS antwika_holdem)

set_target_properties(antwika_holdem PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_include_directories(antwika_holdem
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS antwika_holdem
    EXPORT antwika_holdemTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT antwika_holdemTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)
