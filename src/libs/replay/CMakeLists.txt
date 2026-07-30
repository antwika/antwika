find_package(nlohmann_json REQUIRED)
find_package(nlohmann_json_schema_validator REQUIRED)

add_library(antwika_replay
    src/EngineLoop.cpp
    src/EventJson.cpp
    src/EventSchema.cpp
    src/ReplayCli.cpp
    src/ReplayJson.cpp
    src/ReplayOutput.cpp
    src/ReplayReader.cpp
    src/ReplaySource.cpp
    src/ReplayWriter.cpp
    src/TickPacer.cpp
    src/WindowInputSource.cpp
)

add_library(antwika::replay ALIAS antwika_replay)

set_target_properties(antwika_replay PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_include_directories(antwika_replay
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# antwika::ecs comes in for TickPacer, which is an ecs::ISystem,
# antwika::gfx for WindowInputSource and for the gfx::Size a replay
# records its canvas as, and antwika::log for the one thing this library
# reports rather than throws: a recording made against another canvas.
# All three are one-way: none of those libraries knows this one exists.
target_link_libraries(antwika_replay
    PUBLIC
        antwika::ecs
        antwika::engine
        antwika::event
        antwika::gfx
        antwika::log
        antwika::time
)

target_link_libraries(antwika_replay
    PUBLIC
        nlohmann_json::nlohmann_json
        nlohmann_json_schema_validator
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS antwika_replay
    EXPORT antwika_replayTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT antwika_replayTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)
