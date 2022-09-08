cmake_minimum_required(VERSION 2.8)

add_executable(TEST_fileSize fileSize/fileSize.cpp)

target_compile_definitions(TEST_fileSize PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

target_link_libraries(TEST_fileSize config)

add_test(NAME TEST_fileSize COMMAND TEST_fileSize)


add_executable(TEST_readFile readFile/readFile.cpp)

target_compile_definitions(TEST_readFile PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

target_link_libraries(TEST_readFile config)

add_test(NAME TEST_readFile COMMAND TEST_readFile)