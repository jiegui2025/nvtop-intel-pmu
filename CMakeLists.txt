cmake_minimum_required(VERSION 3.24)

project(nvtop_intel_pmu VERSION 0.1.0 LANGUAGES C)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "intel_pmu uses the Linux i915 PMU and perf_event_open")
endif()

add_library(intel_pmu STATIC src/intel_pmu.c)
add_library(intel_pmu::intel_pmu ALIAS intel_pmu)
set_target_properties(intel_pmu PROPERTIES
  C_STANDARD 11
  C_STANDARD_REQUIRED YES
  C_EXTENSIONS YES
  POSITION_INDEPENDENT_CODE ON
)
target_compile_definitions(intel_pmu PRIVATE _GNU_SOURCE)
target_include_directories(intel_pmu PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include>
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(intel_pmu PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
  enable_language(CXX)
  include(GoogleTest)
  find_package(GTest QUIET)
  if(NOT GTest_FOUND)
    include(FetchContent)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(googletest)
  endif()

  add_executable(intelPmuTests tests/intelPmuTests.cpp)
  target_link_libraries(intelPmuTests PRIVATE intel_pmu::intel_pmu GTest::gtest_main)
  gtest_discover_tests(intelPmuTests)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(intel_pmu_cmake_install_dir ${CMAKE_INSTALL_LIBDIR}/cmake/intel_pmu)
configure_package_config_file(
  cmake/intel_pmu-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/intel_pmu-config.cmake
  INSTALL_DESTINATION ${intel_pmu_cmake_install_dir}
)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/intel_pmu-config-version.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(TARGETS intel_pmu EXPORT intel_pmu_targets)
install(FILES src/intel_pmu.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT intel_pmu_targets
  FILE intel_pmu-targets.cmake
  NAMESPACE intel_pmu::
  DESTINATION ${intel_pmu_cmake_install_dir}
)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/intel_pmu-config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/intel_pmu-config-version.cmake
  DESTINATION ${intel_pmu_cmake_install_dir}
)
