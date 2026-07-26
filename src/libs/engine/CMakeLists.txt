add_library(antwika_engine
    src/Engine.cpp
)

add_library(antwika::engine ALIAS antwika_engine)

set_target_properties(antwika_engine PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_include_directories(antwika_engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_engine
    PUBLIC
        antwika::log
        antwika::event
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS antwika_engine
    EXPORT antwika_engineTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT antwika_engineTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)
