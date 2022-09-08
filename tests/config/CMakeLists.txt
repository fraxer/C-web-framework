cmake_minimum_required(VERSION 2.8)

add_executable(TEST_getConfigPath getConfigPath.cpp)

target_link_libraries(TEST_getConfigPath config)

add_test(NAME TEST_getConfigPath COMMAND TEST_getConfigPath)



add_executable(TEST_loadConfig loadConfig/loadConfig.cpp)

target_compile_definitions(TEST_loadConfig PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

target_link_libraries(TEST_loadConfig config)

add_test(NAME TEST_loadConfig COMMAND TEST_loadConfig)



add_executable(TEST_saveConfigPath saveConfigPath.cpp)

target_link_libraries(TEST_saveConfigPath config)

add_test(NAME TEST_saveConfigPath COMMAND TEST_saveConfigPath)



add_executable(TEST_reloadConfig reloadConfig.cpp)

target_link_libraries(TEST_reloadConfig config)

add_test(NAME TEST_reloadConfig COMMAND TEST_reloadConfig)
