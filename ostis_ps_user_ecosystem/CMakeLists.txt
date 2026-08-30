cmake_minimum_required(VERSION 3.24)
project(ostis_ps_user_ecosystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED SC_MACHINE_BUILD_DIR)
    set(SC_MACHINE_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../sc-machine/build/Release)
endif()

list(APPEND CMAKE_PREFIX_PATH
    ${SC_MACHINE_BUILD_DIR}
    ${SC_MACHINE_BUILD_DIR}/generators
)

find_package(sc-machine REQUIRED)
find_package(GTest REQUIRED)

if(NOT DEFINED SC_EXTENSIONS_DIRECTORY)
    set(SC_EXTENSIONS_DIRECTORY ${CMAKE_BINARY_DIR}/extensions)
endif()
file(MAKE_DIRECTORY ${SC_EXTENSIONS_DIRECTORY})

file(GLOB_RECURSE TRUST_MODULE_SOURCES CONFIGURE_DEPENDS
    "src/*.cpp" "src/*.hpp"
)

add_library(ostis-ps-trust SHARED ${TRUST_MODULE_SOURCES})
target_link_libraries(ostis-ps-trust PUBLIC sc-machine::sc-memory)
target_include_directories(ostis-ps-trust PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
set_target_properties(ostis-ps-trust PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${SC_EXTENSIONS_DIRECTORY})

enable_testing()
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "tests/*.cpp")
add_executable(ostis-ps-trust-tests ${TEST_SOURCES})
target_link_libraries(ostis-ps-trust-tests
    PRIVATE GTest::gtest_main
    PRIVATE ostis-ps-trust
)
target_include_directories(ostis-ps-trust-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

include(GoogleTest)
gtest_discover_tests(ostis-ps-trust-tests)
