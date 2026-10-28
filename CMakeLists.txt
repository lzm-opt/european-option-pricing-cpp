cmake_minimum_required(VERSION 3.24)
project(european_option_pricing VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<1:bin>")
include(CTest)
include(FetchContent)
find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
option(OPTION_BUILD_BENCHMARKS "Build analytic versus CRR pricing benchmarks" ON)
option(OPTION_BUILD_DOCS "Generate checked Doxygen documentation" ON)

add_library(option_core src/pricing.cpp)
target_include_directories(option_core PUBLIC "${PROJECT_SOURCE_DIR}/include")
if(MSVC)
    target_compile_options(option_core PRIVATE /W4)
else()
    target_compile_options(option_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_executable(option_pricer src/main.cpp)
target_link_libraries(option_pricer PRIVATE option_core)

if(BUILD_TESTING)
    set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
    set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)
    FetchContent_Declare(doctest
        URL "${PROJECT_SOURCE_DIR}/third_party/archives/doctest-2.5.3.tar.gz"
        URL_HASH SHA256=174ebc4e769928959614789c5b4e9c3d0a0f81a62bb608756b127bfebfb21331
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(doctest)
    add_executable(option_tests tests/test_pricing.cpp)
    target_link_libraries(option_tests PRIVATE option_core doctest::doctest)
    include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")
    doctest_discover_tests(option_tests)
    add_test(NAME cli_decimal_oracle
        COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
                "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/check_cli.py"
                --app "$<TARGET_FILE:option_pricer>"
                --data-dir "${PROJECT_SOURCE_DIR}/data"
                --convergence-output "${CMAKE_BINARY_DIR}/convergence.json")
    set_tests_properties(cli_decimal_oracle PROPERTIES TIMEOUT 120)
    add_test(NAME workflow_evidence_checks
        COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
                "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/test_workflow_tools.py")
    set_tests_properties(workflow_evidence_checks PROPERTIES TIMEOUT 30)
endif()

if(OPTION_BUILD_BENCHMARKS)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_DOWNLOAD_DEPENDENCIES OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(benchmark
        URL "${PROJECT_SOURCE_DIR}/third_party/archives/benchmark-1.9.5.tar.gz"
        URL_HASH SHA256=9631341c82bac4a288bef951f8b26b41f69021794184ece969f8473977eaa340
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(benchmark)
    add_executable(option_benchmark benchmarks/benchmark_pricing.cpp)
    target_link_libraries(option_benchmark PRIVATE option_core benchmark::benchmark)
    add_custom_target(run_benchmarks
        COMMAND "$<TARGET_FILE:option_benchmark>" --benchmark_min_time=0.02s
                --benchmark_repetitions=3
                "--benchmark_out=${CMAKE_BINARY_DIR}/benchmark.json"
                --benchmark_out_format=json
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/check_benchmark.py"
                "${CMAKE_BINARY_DIR}/benchmark.json"
        DEPENDS option_benchmark
        USES_TERMINAL VERBATIM
        COMMENT "Compare analytic Black-Scholes and 512-step CRR pricing; validate JSON")
endif()

if(OPTION_BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    configure_file(Doxyfile.in "${CMAKE_BINARY_DIR}/Doxyfile" @ONLY)
    add_custom_target(docs
        COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_BINARY_DIR}/Doxyfile"
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/check_docs.py"
                "${CMAKE_BINARY_DIR}/docs/html"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        USES_TERMINAL VERBATIM
        COMMENT "Generate Doxygen HTML and validate local links")
endif()
