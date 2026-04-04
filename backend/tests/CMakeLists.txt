find_package(GTest REQUIRED)

include(GoogleTest)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS *.cpp)

add_executable(nyx-tests ${TEST_SOURCES})

target_link_libraries(nyx-tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    Drogon::Drogon
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    simdjson::simdjson
    jwt-cpp::jwt-cpp
    unofficial-sodium::sodium
)

target_include_directories(nyx-tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

gtest_discover_tests(nyx-tests)
