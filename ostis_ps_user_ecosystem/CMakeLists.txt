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

# --------------------------------------------------------------------------
# Isolated KB copy for tests/test_load_real_kb.cpp (AGENTS_AUDIT.md §7.7,
# item 1: "load the real KB and resolve every keynode"). Built by sc-builder
# into ${CMAKE_BINARY_DIR}/kb_test_repo -- INSIDE the build tree, distinct
# from the production ostis_ps_user_ecosystem/repo/ that sc-server actually
# serves. Rebuilt (via --clear) whenever ostis_kb_user_ecosystem sources
# change; the test itself loads this copy with clear=SC_FALSE and shuts
# down with saveState=SC_FALSE, so it never mutates or erases it either --
# see tools/kb_baseline_counter.cpp and tests/test_load_real_kb.cpp for why
# a plain IsElement()-style check cannot substitute for this comparison
# (ScKeynode's own resolution is find-OR-CREATE, not find-or-fail).
# --------------------------------------------------------------------------

set(OSTIS_KB_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ostis_kb_user_ecosystem)
set(KB_TEST_REPO_DIR ${CMAKE_BINARY_DIR}/kb_test_repo)
set(KB_TEST_CONFIG_HEADER_DIR ${CMAKE_BINARY_DIR}/generated)
set(KB_TEST_CONFIG_HEADER ${KB_TEST_CONFIG_HEADER_DIR}/kb_test_repo_config.hpp)
file(MAKE_DIRECTORY ${KB_TEST_CONFIG_HEADER_DIR})

# sc-machine_DIR is already resolved by find_package(sc-machine REQUIRED)
# above regardless of how CMAKE_PREFIX_PATH ended up pointing at it --
# SC_MACHINE_BUILD_DIR's own documented default does not actually resolve
# for every checkout layout (e.g. this one), so deriving sc-builder's
# location from the package dir that already succeeded is more reliable
# than repeating that same guess a second time.
find_program(SC_BUILDER_EXECUTABLE sc-builder
    HINTS ${sc-machine_DIR}/bin ${SC_MACHINE_BUILD_DIR}/bin
)

add_executable(kb_baseline_counter tools/kb_baseline_counter.cpp)
target_link_libraries(kb_baseline_counter PRIVATE sc-machine::sc-memory)

if(SC_BUILDER_EXECUTABLE)
    file(GLOB_RECURSE OSTIS_KB_SOURCES CONFIGURE_DEPENDS "${OSTIS_KB_SOURCE_DIR}/*.scs")
    add_custom_command(
        OUTPUT ${KB_TEST_CONFIG_HEADER}
        COMMAND ${SC_BUILDER_EXECUTABLE} -i ${OSTIS_KB_SOURCE_DIR} -o ${KB_TEST_REPO_DIR} --clear
        COMMAND kb_baseline_counter ${KB_TEST_REPO_DIR} ${KB_TEST_CONFIG_HEADER}
        DEPENDS kb_baseline_counter ${OSTIS_KB_SOURCES}
        COMMENT "Building isolated KB copy for test_load_real_kb (not the production repo/) and its baseline element count"
        VERBATIM
    )
    add_custom_target(generate_kb_test_config DEPENDS ${KB_TEST_CONFIG_HEADER})
else()
    message(WARNING "sc-builder not found (looked near ${sc-machine_DIR}/bin and ${SC_MACHINE_BUILD_DIR}/bin) -- test_load_real_kb.cpp will report SKIPPED at runtime")
    file(WRITE ${KB_TEST_CONFIG_HEADER}
        "#pragma once\n#define KB_TEST_REPO_AVAILABLE 0\n#define KB_TEST_REPO_DIR \"\"\n#define KB_TEST_REPO_BASELINE_ELEMENT_COUNT 0ULL\n"
    )
    add_custom_target(generate_kb_test_config)
endif()

file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "tests/*.cpp")
add_executable(ostis-ps-trust-tests ${TEST_SOURCES})
target_link_libraries(ostis-ps-trust-tests
    PRIVATE GTest::gtest_main
    PRIVATE ostis-ps-trust
)
target_include_directories(ostis-ps-trust-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(ostis-ps-trust-tests PRIVATE ${KB_TEST_CONFIG_HEADER_DIR})
add_dependencies(ostis-ps-trust-tests generate_kb_test_config)

include(GoogleTest)
gtest_discover_tests(ostis-ps-trust-tests)
