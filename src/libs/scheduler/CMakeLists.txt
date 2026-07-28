add_library(antwika_scheduler
    src/Scheduler.cpp
)

add_library(antwika::scheduler ALIAS antwika_scheduler)

set_target_properties(antwika_scheduler PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_include_directories(antwika_scheduler
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_scheduler
    PUBLIC
        antwika::time
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS antwika_scheduler
    EXPORT antwika_schedulerTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT antwika_schedulerTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)
