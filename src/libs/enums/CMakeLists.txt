add_library(antwika_enums INTERFACE)

add_library(antwika::enums ALIAS antwika_enums)

target_include_directories(antwika_enums
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS antwika_enums
    EXPORT antwika_enumsTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT antwika_enumsTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)
