find_package(GTest REQUIRED)

include(GoogleTest)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS *.cpp)

set(TESTABLE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/application/auth/AuthService.cpp
    ${CMAKE_SOURCE_DIR}/src/application/profile/ProfileService.cpp
    ${CMAKE_SOURCE_DIR}/src/application/equipment/EquipmentService.cpp
    ${CMAKE_SOURCE_DIR}/src/application/location/LocationService.cpp
)

add_executable(nyx-tests ${TEST_SOURCES} ${TESTABLE_SOURCES})

target_link_libraries(nyx-tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    Drogon::Drogon
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    simdjson::simdjson
    jwt-cpp::jwt-cpp
    unofficial-sodium::sodium
    nlohmann_json_schema_validator::validator
)

target_include_directories(nyx-tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

gtest_discover_tests(nyx-tests)
