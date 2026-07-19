cmake_minimum_required(VERSION 3.16)
project(ClickEncoderTests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Catch2 3 REQUIRED)

add_executable(run_tests
    test_encoder.cpp
    ../src/ClickEncoder.cpp
)

target_include_directories(run_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/../src
    ${PROJECT_SOURCE_DIR}/stubs
)

target_link_libraries(run_tests PRIVATE Catch2::Catch2WithMain)
