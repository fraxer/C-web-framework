cmake_minimum_required(VERSION 2.8)


add_executable(TEST_getPath getPath.cpp)

target_link_libraries(TEST_getPath config common)

add_test(NAME TEST_getPath COMMAND TEST_getPath)



add_executable(TEST_load load.cpp)

#target_compile_definitions(TEST_load PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

target_link_libraries(TEST_load config common)

add_test(NAME TEST_load COMMAND TEST_load)



add_executable(TEST_savePath savePath.cpp)

target_link_libraries(TEST_savePath config common)

add_test(NAME TEST_savePath COMMAND TEST_savePath)



add_executable(TEST_reload reload.cpp)

#target_compile_definitions(TEST_reload PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

target_link_libraries(TEST_reload config common)

add_test(NAME TEST_reload COMMAND TEST_reload)
