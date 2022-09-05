cmake_minimum_required(VERSION 2.8)

add_executable(getPathConfigFile getPathConfigFile.cpp)

target_link_libraries(getPathConfigFile config)

add_test(NAME getPathConfigFile COMMAND getPathConfigFile)



add_executable(loadConfig loadConfig.cpp)

target_link_libraries(loadConfig config)

add_test(NAME loadConfig COMMAND loadConfig)
