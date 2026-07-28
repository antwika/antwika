add_library(antwika_ecs
    src/EntityManager.cpp
)

add_library(antwika::ecs ALIAS antwika_ecs)

set_target_properties(antwika_ecs PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_include_directories(antwika_ecs
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_ecs
    PUBLIC
        antwika::time
        antwika::log
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS antwika_ecs
    EXPORT antwika_ecsTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT antwika_ecsTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)
