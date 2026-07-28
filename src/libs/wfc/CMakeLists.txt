add_library(antwika_wfc
    src/Domain.cpp
    src/AllDifferentConstraint.cpp
    src/CompatibilityTable.cpp
    src/AdjacencyConstraint.cpp
    src/Trail.cpp
    src/EntropyIndex.cpp
    src/Solver.cpp
)

add_library(antwika::wfc ALIAS antwika_wfc)

set_target_properties(antwika_wfc PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_include_directories(antwika_wfc
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS antwika_wfc
    EXPORT antwika_wfcTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT antwika_wfcTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)
